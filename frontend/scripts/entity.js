
async function fetchApi (endpoint, body = {}) {
	let args = {
		"method": endpoint.method,
		"credentials": 'same-origin'
	};
	if (endpoint.method != "GET") {
		args['body'] = JSON.stringify (body);
	}

	let fRes = await fetch (endpoint.uri, args);

	if (fRes.ok) {
		return await fRes.json();
	} else {
		console.log (await fRes.text());
		return null;
	}
}

function dateToString (date) {
	let abc = date.split ("-");
	return abc[2] + "." + abc[1] + "." + abc[0];
}

function getFont (elem) {
	// let inp = this.inputTemplate.querySelector ('input');
	let css = window.getComputedStyle (elem);
	let weight = css ['font-weight'];
	let size = css ['font-size'];
	let family = css ['font-family'];
	let font = `${weight} ${size} ${family}`;
	return font;
}

var testingCanvas = null;
// with help from https://stackoverflow.com/a/21015393/8665933
function getTextWidth (text, elem) {
	if (testingCanvas == null) {
		testingCanvas = document.createElement ('canvas');
	}
	let ctx = testingCanvas.getContext ('2d');
	ctx.font = getFont (elem);
	return ctx.measureText (text).width;
}

function flashNetworkError () {
	// TODO: this
	console.log ("FlashNetworkError!");
}

function intDateEval (date) {
	let abc = date.split ('-');
	for (let i in abc) abc[i] = parseInt (abc[i]);
	return abc[0] * 10000 + abc[1] * 100 + abc[2];
}

function compareDates (a, b) {
	return intDateEval (a) - intDateEval (b);
}

function getEntityUrl (id){ 
	return '/e?id=' + id;
}

function getUrlForCreation (text) {
	let spl = text.split (" ");
	if (spl.length == 0) return "/create";
	let res = "/create?q=" + spl[0];
	for (let i = 1; i < spl.length; i++) {
		if (spl[i] == "") continue;
		res += "+" + spl[i];	
	}
	return res;
}



class EntityDataView {
	constructor (entity) {
		this.entity = entity;
		this.defaultPicturePath = '/resources/default_picture.svg';
	}

	initElements () {
		this.entityNameElem = document.getElementById ("personName");
		this.entityDatesElem = document.getElementById ("personLifetime");
		this.entityDescriptionElem = document.getElementById ("personBio");
		this.entityPicElem = document.getElementById ("personImage");
	}

	formDate (obj) {
		let str = "" + dateToString(obj.start_date);
		if ('end_date' in obj) str += " - " + dateToString(obj.end_date);
		return str;
	}

	getPicturePath (obj) {
		if ('picture_path' in obj) return obj['picture_path'];
		return this.defaultPicturePath;
	}

	updateElements () {
		let entityData = this.entity.entityData;
		this.entityNameElem.innerHTML = entityData.name;
		this.entityDatesElem.innerHTML = this.formDate (entityData);
		this.entityDescriptionElem.innerHTML = entityData.description;

		this.entityPicElem.setAttribute ('src', this.getPicturePath (entityData));
		if (entityData.type == 'band') {
			this.entityPicElem.classList.add ('square');
		}
	}

	initialize () {
		this.initElements ();
		this.updateElements ();
	}
}

class EventReporter {
	constructor (eventsView) {
		this.getReportTypesEndpoint = {
			"uri": "/api/events/report_types",
			"method": "GET"
		};
		this.reportEventEndpoint = {
			"uri": "api/events/report",
			"method": "POST"
		};
	}

	async initialize () {
		this.veil = document.getElementById ("reportWindowVeil");
		this.window = document.getElementById ("reportWindow");
		this.cancelButton = document.getElementById ('cancelReport');
		this.submitButton = document.getElementById ('sendReport');
		this.reportReasonTemplate = document.getElementById ("reportTypeOptionTemplate");

		this.reportReasons = await fetchApi (this.getReportTypesEndpoint);
		this.setupElements ();

		this.selectedReason = null;
		this.eventId = null;
	}

	setupElements () {
		for (let i of this.reportReasons) {
			let clone = this.reportReasonTemplate.cloneNode (true);
			clone.id = "";
			clone.classList.remove ('template');
			clone.innerHTML = i.name;
			this.reportReasonTemplate.parentElement.insertBefore (clone, this.reportReasonTemplate);
			clone.addEventListener ('click', (ev) => { this.selectReason (ev, i.id); });
		}

		this.submitButton.addEventListener ('click', (ev) => { this.submit(); });
		this.cancelButton.addEventListener ('click', () => { this.hideWindow(); });
		this.veil.addEventListener ('click', () => { this.hideWindow(); })

		window.addEventListener ('resize', () => { this.resize(); });
	}

	enableSubmitButton () {
		this.submitButton.classList.remove ("inactive");
	}

	disableSubmitButton () {
		this.submitButton.classList.add ('inactive');
	}

	selectReason (ev, id) {
		let opponents = document.querySelectorAll ('.selected.reportTypeOption');

		for (let i of opponents) {
			i.classList.remove ('selected');
		}

		ev.target.classList.add ('selected');
		this.selectedReason = id;
		this.enableSubmitButton ();
	}

	resize() {
		let wbcr = this.window.getBoundingClientRect();
		let pbcr = {
			width: window.innerWidth,
			height: window.innerHeight
		};
		this.window.style.left = ((pbcr.width - wbcr.width) / 2) + "px";
		this.window.style.top = ((pbcr.height - wbcr.height) / 2) + "px";
	}
	
	showWindow () {
		this.veil.classList.add ("ondisplay");
		this.window.classList.add ("ondisplay");
		this.veil.classList.remove ("hiddenRep");
		this.window.classList.remove ("hiddenRep");
		this.resize();
	}

	hideWindow () {
		this.veil.classList.add ("hiddenRep");
		this.window.classList.add ("hiddenRep");
		this.eventId = null;

		setTimeout (() => {
			this.veil.classList.remove ("ondisplay");
			this.window.classList.remove ("ondisplay");
		}, 200);
		this.reset();
	}

	reset () {
		this.selectedReason = null;
		let opponents = document.querySelectorAll ('.selected.reportTypeOption');
		for (let i of opponents) {
			i.classList.remove ('selected');
		}
		this.disableSubmitButton ();
	}

	reportEvent (eventId) {
		this.eventId = eventId;
		this.showWindow ();
	}

	async submit () {
		if (this.selectedReason === null) return;
		fetchApi (this.reportEventEndpoint, {"event_id": parseInt(this.eventId), "reason_id": parseInt(this.selectedReason)});
		this.hideWindow ();
	}
}


class EventDisplayObject {
	constructor (event, eventsView) {
		this.event = event;
		this.eventsView = eventsView;
		this.nextEvent = null;

		this.picsDir = '/resources/timeline/';
		this.typePics = {
			'band_foundation': 'band.svg',
			'band_leave': 'band.svg',
			'band_join': 'band.svg',
			'ep': 'song.svg',
			'album': 'album.svg'
		};
		this.defaultPic = 'song.svg';

		this.constructElem ();
	}

	formatString (str, data) { 
		const getVariablesRegex = /{[^\{\}]*}/gm;
		let vars = [...str.matchAll (getVariablesRegex)];
		for (let j of vars) {
			let i = j[0];
			let varName = ("" + i).slice (1, -1); // 'abc' from '{abc}'
			if (varName in data) {
				let variable = data [varName];
				if (typeof variable === 'object' && variable !== null && !Array.isArray(variable)) {
					if ('name' in variable) {
						let url = null;
						if ('url' in variable) {
							url = variable.url;
						} else if ('entity_id' in variable) {
							url = getEntityUrl (variable.entity_id);
						}

						let repl = '<a' + ((url != null) ? (' href = "' + url + '"') : "") + ">" + variable.name + "</a>";
						str = str.replaceAll (i, repl);
					} else {
						console.log ("Can't convert object to string: ");
						console.log (variable);
					}
				} else {
					str = str.replaceAll (i, variable);
					console.log (data);
					console.log (`${i} -> ${variable}`);
				}
			} else {
				console.log ("No variable named '" + varName + "'(from " + i + ") in data object: ");
				console.log (data);
			}
		}
		return str;
	} 
	
	getDateString (event) {
		let str = dateToString (event.start_date);
		if ('end_date' in event) return str + ' - ' + dateToString (event.end_date);
		return str;
	}

	getPictureForType (type) {
		if (type in this.typePics) {
			return this.picsDir + this.typePics [type];
		}
		return this.picsDir + this.defaultPic;
	}

	setupCallbacks () {
		this.elem.querySelector ('.' + this.eventsView.lookupEntryClass).addEventListener ('click', () => { this.lookupEntry(); });
		this.elem.querySelector ('.' + this.eventsView.reportEntryClass).addEventListener ('click', () => { this.reportEntry(); });
		this.elem.querySelector ('.' + this.eventsView.editEntryClass).addEventListener ('click', () => { this.editEntry(); });
	}

	getTitle () {
		return this.elem.querySelector ('.' + this.eventsView.eventTitleClass).innerText;
	}

	genLookupUrl () {
		let keys = this.getTitle().split (' ');
		if (keys.length == 0) return "https://developer.mozilla.org/en-US/docs/Web/JavaScript";
		let res = "https://www.google.com/search?q=" + keys[0];
		for (let i = 1; i < keys.length; i++) res += "+" + keys[i];
		return res;
	}

	lookupEntry () {
		window.open (this.genLookupUrl (), '_blank', 'noopener');
	}

	reportEntry () {
		this.eventsView.eventReporter.reportEvent (this.event.id);
	}

	editEntry () {

	}

	generateParticipant (pc) {
		let clone = this.eventsView.eventParticipantTemplate.cloneNode (true);
		clone.innerHTML = pc.name;
		clone.classList.remove ('template');
		clone.id = '';
		if (pc.created) {
			clone.addEventListener ('click', () => { location.href = getEntityUrl (pc.entity_id)});
		} else {
			clone.classList.add ('notCreated');
			clone.addEventListener ('click', () => { if (demandAuth()) location.href = getUrlForCreation (pc.name)});
		}
		this.elem.querySelector ('.' + this.eventsView.eventParticipantBoxClass).appendChild (clone);
	}

	generateParticipants () {
		for (let i of this.event.participants) {
			// Don't show current entity as a participant
			if (i.entity_id != this.eventsView.entity.entityData.entity_id)
				this.generateParticipant (i)
		}
	}

	constructElem () {
		let clone = this.eventsView.eventBodyTemplate.cloneNode (true);
		clone.id = 'event_' + this.event.id;
		clone.classList.remove ('template'); 
		
		this.title = this.formatString (this.event.title, this.event.data);

		clone.querySelector ('.' + this.eventsView.eventTitleClass).innerHTML = this.title;
		clone.querySelector ('.' + this.eventsView.eventTimepointClass).innerHTML = this.getDateString (this.event);
		clone.querySelector ('.' + this.eventsView.eventDescriptionClass).innerHTML = this.formatString (this.event.description, this.event.data);
		clone.querySelector ('.' + this.eventsView.pictureClass).setAttribute ('src', this.getPictureForType (this.event.type));

		this.elem?.remove();
		this.elem = clone;
		this.setupCallbacks ();
		this.generateParticipants ();
	}

	insertBefore (elem) {
		elem.parentElement.insertBefore (this.elem, elem);
	}

	laterDate () {
		if ('end_date' in this.event) return this.event.end_date;
		return this.event.start_date;
	}

	earlierDate () {
		return this.event.start_date;
	}

	isEarlierThanEvent (event) {
		return compareDates(this.laterDate(), event.earlierDate()) < 0;
	}

	isOrderedBefore (event) {
		if (this.isEarlierThanEvent (event) && event.isEarlierThanEvent (this)) {
			return this.event.order_index < event.order_index;
		}
		return this.isEarlierThanEvent (event);
	}
}

class EventsView {
	constructor (entity) {
		this.entity = entity;

		this.getEventsApiEndpoint = {
			uri: '/api/events/getfor',
			method: 'POST'
		};

		window.addEventListener ('resize', () => { this.resize(); });

		this.firstEvent = null;
		this.events = [];
		this.connectors = [];

		this.eventReporter = new EventReporter;
	}

	insertEvent (event) {
		let prev = null;
		let ptr = this.firstEvent;

		while (ptr != null) {
			if (event.isOrderedBefore (ptr)) {
				event.insertBefore (ptr.elem);
				if (ptr === this.firstEvent) this.firstEvent = event;
				if (prev !== null) prev.next = event;
				event.next = ptr;
				return;
			}
			prev = ptr;
			ptr = ptr.next;
		}

		if (this.firstEvent == null) {
			this.firstEvent = event;
		} else {
			prev.next = event;
		}
		event.insertBefore (this.eventPointer);
	}

	resize () {
		console.log ('resize');
		this.reconnect();
	}

	constructEvent (event) {
		console.log (event);
		let newEventObj = new EventDisplayObject (event, this);
		this.events.push (newEventObj);
		this.insertEvent (newEventObj);
		this.reconnect();
	}

	genConnector (topPoint, bottomPoint) {
		let clone = this.connectorTemplate.cloneNode (true);

		clone.id = '';
		clone.classList.remove ('template');
		clone.classList.add ('actualConnector');
		let line = clone.querySelector ('line');

		clone.style.left = (topPoint.x - 4) + 'px';
		clone.style.top = topPoint.y + 'px';
		clone.setAttribute ('height', (bottomPoint.y - topPoint.y) + 'px');
		line.setAttribute('y2', bottomPoint.y - topPoint.y);
		this.connectorTemplate.parentElement.appendChild (clone);
	}
	
	getElemBottomCoordsForConnector (elem) {
		let br = elem.getBoundingClientRect ();
		return {
			x: (br.left + br.right) / 2 + window.scrollX,
			y: br.bottom - 16 + window.scrollY
		};
	}
	
	getElemTopCoordsForConnector (elem) {
		let br = elem.getBoundingClientRect ();
		return {
			x: (br.left + br.right) / 2 + window.scrollX,
			y: br.top + 16 + window.scrollY
		};
	}

	connect (a, b) {
		let aIcon = a.querySelector('.timelineElemIcon');
		let bIcon = b.querySelector('.timelineElemIcon');
	
		let topPoint = this.getElemBottomCoordsForConnector (aIcon);
		let bottomPoint = this.getElemTopCoordsForConnector (bIcon);
	
		this.genConnector (topPoint, bottomPoint);
	}
	
	generateConnectors () {
		let nodes = document.querySelectorAll ('.timelineElem');
		let elems = [];

		for (let i of nodes) { // Not connecting templates
			if (!i.classList.contains ('template')) {
				elems.push (i);
			}
		}

		for (let i = 0; i < elems.length - 1; i++) {
			this.connect (elems[i], elems[i + 1]);
		}
	}
	
	reconnect (ev) {
		let connectors = document.querySelectorAll ('.actualConnector');
		this.generateConnectors ();
		for (let i of connectors) {
			i.remove();
		}
	}

	async pullEvents () {
		let pull = await fetchApi (this.getEventsApiEndpoint, {'id': this.entity.id});
		return pull.events;
	}

	async initialize () {
		this.eventBodyTemplate = document.getElementById ('timelineElemTemplate');
		this.eventTitleClass = 'timepointTitle';
		this.eventTimepointClass = 'timepoint';
		this.eventDescriptionClass = 'timepointBody';
		this.pictureClass = 'timelineElemIcon';
		this.eventParticipantBoxClass = 'eventParticipants';
		this.eventParticipantTemplate = document.getElementById('eventParticipantTemplate');

		this.reportEntryClass = 'reportEntry';
		this.editEntryClass = 'editEntry';
		this.lookupEntryClass = 'lookupEntry';

		this.connectorTemplate = document.getElementById ('timelineConnectorTemplate');

		this.eventPointer = document.getElementById ('latestEventHere');

		this.types = this.entity.eventTypes;
		let events = await this.pullEvents();
		for (let i of events) {
			this.constructEvent (i);
		}

		this.eventReporter.initialize();
	}
}




/**************************************************************************************************
 **************************************************************************************************
 * 
 * Events info input
 * 
 **************************************************************************************************
 **************************************************************************************************/


class AbstractEventInfoInput {
	constructor (input, factory) {}
	getValue () {}
	getId () {}
	isValid () {}
	getRootElem () {}
	onchange (callback) {}
}

class TextFieldInput {
	constructor (input, factory) {
		this.id = input.id;
		this.optional = input.optional;
		this.prompt = input.prompt;
		
		this.factory = factory;
		this.template = document.getElementById ('textInputTemplate');
		this.promptSubclass = 'creationInputLabel';
		this.inputSubclass = 'creationInput';

		this.generateElement ();
	}

	generateElement () {
		this.elem = this.template.cloneNode (true);
		this.elem.id = this.id;
		this.elem.classList.remove ('template');
		this.inputElem = this.elem.querySelector ('.' + this.inputSubclass);
		this.promptElem = this.elem.querySelector ('.' + this.promptSubclass);
		this.promptElem.innerHTML = this.prompt;

		this.factory.insertInputElem (this.elem);
	}

	getValue () {
		return this.inputElem.value;
	}

	getId () {
		return this.id;
	}

	isValid () {
		return this.optional || (this.getValue() != '');
	}

	getRootElem () {
		return this.elem;
	}

	onchange (callback) {
		this.inputElem.addEventListener ('input', callback);
	}
}


class CurrentEntityInput {
	constructor (input, factory) {
		this.id = input.id;
		this.factory = factory;
	}

	getValue () {
		return {
			"created": true,
			"name": this.factory.entity.entityData.name,
			"entity_id": parseInt(this.factory.entity.entityData.entity_id)
		};
	}

	getId () {
		return this.id;
	}

	isValid () {
		return true;
	}

	getRootElem () {
		return null;
	}

	onchange (callback) {
		// ...
	}
}


class DateInput {
	constructor (input, factory) {
		this.id = input.id;
		this.optional = input.optional;
		this.prompt = input.prompt;
		
		this.factory = factory;
		this.template = document.getElementById ('dateInputTemplate');
		this.promptSubclass = 'creationInputLabel';
		this.inputSubclass = 'creationInput';

		this.generateElement ();
	}

	generateElement () {
		this.elem = this.template.cloneNode (true);
		this.elem.id = this.id;
		this.elem.classList.remove ('template');
		this.inputElem = this.elem.querySelector ('.' + this.inputSubclass);
		this.promptElem = this.elem.querySelector ('.' + this.promptSubclass);
		this.promptElem.innerHTML = this.prompt;

		this.factory.insertInputElem (this.elem);
	}

	getValue () {
		return this.inputElem.value;
	}

	getId () {
		return this.id;
	}

	isValid () {
		return this.optional || (this.getValue() != '');
	}

	getRootElem () {
		return this.elem;
	}

	onchange (callback) {
		this.inputElem.addEventListener ('input', callback);
	}
};

class TextareaInput {
	constructor (input, factory) {
		this.id = input.id;
		this.optional = input.optional;
		this.prompt = input.prompt;
		
		this.factory = factory;
		this.template = document.getElementById ('textareaInputTemplate');
		this.promptSubclass = 'creationInputLabel';
		this.inputSubclass = 'creationInput';

		this.generateElement ();
	}

	generateElement () {
		this.elem = this.template.cloneNode (true);
		this.inputElem = this.elem.querySelector ('.' + this.inputSubclass);
		this.promptElem = this.elem.querySelector ('.' + this.promptSubclass);
		this.elem.id = this.id;
		this.elem.classList.remove ('template');
		this.promptElem.innerHTML = this.prompt;

		this.factory.insertInputElem (this.elem);
	}

	getValue () {
		return this.inputElem.value;
	}

	getId () {
		return this.id;
	}

	isValid () {
		return this.optional || (this.getValue() != '');
	}

	getRootElem () {
		return this.elem;
	}
	
	onchange (callback) {
		this.inputElem.addEventListener ('input', callback);
	}
}

class BandSearchSuggestions {
	constructor (parent) {
		this.parent = parent;
		this.cache = {};
		this.currentOption = null;
		this.elem = null;

		window.addEventListener ('resize', () => {this.setupSuggestionListPosition();});
	}

	setupSuggestionListPosition () {
		if (this.elem == null) return;
		let box = this.parent.inputElem.getBoundingClientRect();
		this.elem.style.top = (box.bottom + window.scrollY) + "px";
		this.elem.style.left = (box.left + window.scrollX) + "px";
		this.elem.style.width = box.width + "px";
	}

	createSearchSuggestion (optTemplate, elem, opt, index) {
		let clone = optTemplate.cloneNode (true);
		
		clone.innerHTML = opt.title;
		clone.classList.remove ("template");
		clone.id = elem.id + "_ss_" + index;

		clone.addEventListener ('click', (ev) => { this.selectOpt (ev.target, elem, opt); });

		return clone;
	}

	constuctList () {
		this.elem?.remove();
		this.elem = this.parent.templates.suggestionList.cloneNode (true);
		this.elem.classList.remove ('template');

		this.parent.templates.suggestionList.parentElement.insertBefore (this.elem, this.parent.templates.suggestionList);
	}

	clearList () {
		if (this.elem == null) return;
		for (let i = this.elem.childElementCount - 1; i >= 0; i--) {
			this.elem.children[i].remove();
		}
	}

	addSuggestion (s) {
		let clone = this.parent.templates.suggestion.cloneNode (true);
		clone.classList.remove ('template');
		clone.innerHTML = s.title;
		clone.addEventListener ('click', (ev) => { this.pickOption (s) });
		this.elem.appendChild (clone);
	}

	buildList (results) {
		if (this.elem == null) this.constuctList();
		else this.clearList();

		for (let i of results) {
			this.addSuggestion (i);
		}
	}

	suggestFor (results) {
		this.buildList (results);
		this.setupSuggestionListPosition();
	}

	pickOption (option) {
		this.parent.selectItem (option);
		this.hide();
	}

	hide () {
		this.clearList();
		this.elem?.remove();
	}
};

class BandInput {
	constructor (input, factory) {
		this.id = input.id;
		this.factory = factory;

		this.templates = {
			input: document.getElementById ('textInputTemplate'),
			suggestion: document.getElementById ('creationInputSuggestionTemplate'),
			suggestionList: document.getElementById ('creationInputSuggestionListTemplate'),
			labelSelector: ".creationInputLabel"
		};

		this.elem = this.templates.input.cloneNode (true);
		this.elem.id = input.id + "_cont";
		this.elem.classList.remove ('template');
		this.inputElem = this.elem.querySelector ('input');
		
		this.elem.querySelector (this.templates.labelSelector).innerHTML = input.prompt;
	
		this.inputElem.setAttribute ('type', 'text');
		this.inputElem.addEventListener ('input', (ev) => { this.checkBandInput(ev);} );
	
		this.inputElem.id = input.id;
		
		this.factory.insertInputElem (this.elem);

		this.selectedItem = null;
		this.suggestions = new BandSearchSuggestions (this); 
		this.bandSearchCache = {};
		
		this.onchangeCallbacks = [];
	}

	selectItem (item) {
		this.selectedItem = item;
		this.inputElem.value = item.title;
		this.callChangeCallbacks ();
	}

	checkBandInput (ev) {
		this.selectedItem = null;
		
		let checkId =  Math.floor (10000000 * Math.random ());
		this.latestCheckId = checkId;
		let value = this.inputElem.value.trim();

		this.callChangeCallbacks ();

		if (value in this.bandSearchCache) {
			this.suggestions.suggestFor (this.bandSearchCache[value]);
			return;
		}
		
		let body = {
			"prompt": ev.target.value
		};


		fetch (
			'/api/search/b',
			{
				"method": 'POST',
				"body": JSON.stringify (body)
			}
		).then (async (resp) => {
			if (checkId != this.latestCheckId) return;
			let res = await resp.json();
			// console.log ("checkBandInput::res:");
			// console.log (res);
			this.bandSearchCache [value] = res.results;
			this.suggestions.suggestFor (res.results);
		}, /* on rejection */(resp) => {
			if (checkId != this.latestCheckId) return;
		});
	}

	getValue () {
		let ret = {
			created: (this.selectedItem != null),
			name: this.inputElem.value
		};
		if (ret.created) ret.entity_id = this.selectedItem.id; 
		return ret;
	}

	getId () {
		return this.id;
	}

	getRootElem () {
		return this.elem;
	}

	isValid () {
		return this.optional || (this.inputElem.value != '');
	}
	

	callChangeCallbacks () {
		for (let i of this.onchangeCallbacks) {
			i();
		}
	}

	onchange (callback) {
		this.onchangeCallbacks.push (callback);
	}
}


class InputFactory {
	constructor (entity) {
		this.entity = entity;
		this.inputTypes = {};
	}

	initialize () {
		this.inputPointer = document.getElementById ('inputPlacer');
	}

	registerInputType (typeId, classRef) {
		if (typeId in this.inputTypes) {
			console.log ("duplicate typeId in InputFactory::registerInputType: '" + typeId + "'");
			throw "duplicate typeId in InputFactory::registerInputType: '" + typeId + "'";
		}

		this.inputTypes [typeId] = classRef;
	}

	insertInputElem (elem) {
		this.inputPointer.parentElement.insertBefore (
			elem,
			this.inputPointer
		);
	}

	construct (inputType, inputData) {
		return new this.inputTypes [inputType] (inputData, this);
	}
}


class EventGeneratorType {
	constructor (id, type, eventGenerator) {
		this.eventGenerator = eventGenerator;
		this.inputFactory = eventGenerator.inputFactory;

		this.type = type;
		console.log (type);
		this.id = id;
		this.constructElem ();

		this.primaryClassName = 'timelineTypeSelectorButton';
		this.selectedClassName = 'selected';
	}

	getElemId () {
		return this.id + '_type';
	}

	pick () {
		let opponents = document.querySelectorAll ('.' + this.primaryClassName + '.' + this.selectedClassName);
		for (let i of opponents) {
			i.classList.remove (this.selectedClassName);
		}
		this.elem.classList.add (this.selectedClassName);
		this.constuctInputs ();
		this.eventGenerator.currentType = this;
		this.eventGenerator.displayParticipants ();
		this.eventGenerator.checkSubmitButtonActivation ();
		this.eventGenerator.scrollToView ();
	}

	constuctInputs () {
		this.eventGenerator.clearInputs ();
		
		for (let i in this.type.inputs) {
			let inp = this.type.inputs[i];
			let newInp = this.inputFactory.construct (inp.type, inp);
			newInp.onchange (() => {this.inputCallback();});
			this.eventGenerator.inputs.push (newInp);
		}
	}

	constructElem () {
		this.elem?.remove();

		this.elem = this.eventGenerator.timelineTypeSelectorTemplate.cloneNode (true);
		this.elem.innerHTML = this.type.type_display_name;
		this.elem.classList.remove ('template');
		this.elem.id = this.getElemId ();

		this.elem.addEventListener ('click', () => { this.pick(); });

		this.eventGenerator.timelineTypeSelectorTemplate.parentElement.insertBefore (
			this.elem,
			this.eventGenerator.timelineTypeSelectorTemplate
		);
	}

	inputCallback () {
		this.eventGenerator.checkSubmitButtonActivation ();
	}

	inputsValid () {
		for (let i of this.eventGenerator.inputs) {
			if (!i.isValid()) return false;
		}
		return true;
	}

	getValues () {
		let res = {};
		for (let i of this.eventGenerator.inputs) {
			res [i.getId()] = i.getValue();
		}
		return res;
	}

	clear() {
		let opponents = document.querySelectorAll ('.' + this.primaryClassName + '.' + this.selectedClassName);
		for (let i of opponents) {
			i.classList.remove (this.selectedClassName);
		}
	}
}




/***************************************************************************************************
 ***************************************************************************************************
 * 
 *  Participants
 * 
 ***************************************************************************************************
 ***************************************************************************************************/


class ParticipantEntityTemplates {
	constructor (inputTemplate, suggestionListTemplate, suggestionTemplate, minInputElementWidth) {
		this.input = inputTemplate;
		this.suggestionList = suggestionListTemplate;
		this.suggestion = suggestionTemplate;
		this.minInputElementWidth = minInputElementWidth;
	}
}


class ParticipantEntitySuggestionList {
	constructor (entity) {
		this.entity = entity;

		this.elem = null;
		this.hidden = true;

		this.api = '/api/search/entities';
		this.method = 'POST';

		this.requestId = 0;

		this.suggestions = [];
		this.resultsCache = {};
		this.latestResults = [];
		
		window.addEventListener ('resize', (ev) => { this.resize(); });
	}

	selectItem (id) {
		console.log ('Selected id #' + id);
		console.log (this.latestResults[id]);
		this.entity.selectItem (this.latestResults [id]);
	}

	generateListClone () {
		this.elem?.remove();
		this.elem = this.entity.templates.suggestionList.cloneNode (true);
		this.elem.classList.remove ('template');
		this.elem.id = "suggestionList_" + Math.floor (Math.random() * 10000000);
		this.resize();
		this.entity.elem.parentElement.appendChild (this.elem);
	}

	resize () {
		let bbox = this.entity.elem.getBoundingClientRect();
		let thisBox = this.elem.getBoundingClientRect ();
		// Could set this.elem.style.bottom but it leads to .top being equal to -699 for whatever reason and element not being rendered within page bounds
		this.elem.style.top = (bbox.top + window.scrollY - thisBox.height) + "px";
		this.elem.style.left = (bbox.left + window.scrollX) + "px";
	}

	generateSuggestionClone (text, id) {
		let clone = this.entity.templates.suggestion.cloneNode (true);
		clone.classList.remove ('template');
		clone.classList.remove ('hidden');
		clone.innerHTML = text;
		this.elem.insertBefore (clone, this.elem.firstChild);

		clone.addEventListener ('mousedown', (ev) => { this.selectItem (id) })
	}

	buildSuggestions (results) {
		if (results == undefined || results == null || results?.length == 0) {
			this.latestResults = [];
			this.hide();
			return;
		}

		if (this.elem == null) this.generateListClone();
		this.elem.innerHTML = "";
		this.latestResults = results;
		for (let i = 0; i < results.length; i++) {
			this.generateSuggestionClone (results[i].title, i);
		}
		this.elem.classList.remove ('hidden');
		this.resize();
	}

	async suggestFor (prompt) {
		prompt = prompt.trim();
		if (prompt in this.resultsCache) {
			this.buildSuggestions (this.resultsCache [prompt]);
			return;
		}

		let body = { 'prompt': prompt };
		let fetchOpts = {
			body: JSON.stringify (body),
			credentials: 'same-origin',
			method: this.method
		}

		let requestId = Math.floor (Math.random() * 10000000);
		this.requestId = requestId;
		fetch (this.api, fetchOpts).then (
			/* 200 */ async (resp) => {
				if (this.requestId != requestId) return;
				let results = await resp.json();
				this.resultsCache [prompt] = results.results;
				// console.log (results.results);
				this.buildSuggestions (results.results);
			},
			/* Rejected*/ (resp) => {
				console.log (resp);
			}
		);
	}

	hide () {
		// Timeout is required to process 'click' events on suggestions before clearing them
		setTimeout (() => { 
			this.elem?.remove();
			this.elem = null;
			this.latestResults = [];	
		}, 100);
	}
}


class ParticipantEntityInput {
	constructor (prev, id, templates, deletionCallback) {
		this.prev = prev;
		this.next = null;
		this.prev?.setNext (this);
		
		this.id = id;
		this.templates = templates;
		this.deletionCallback = deletionCallback;
		this.makeSelf();

		this.suggestionList = new ParticipantEntitySuggestionList (this);
		this.selectedId = null;
	}
	
	setNext (next) {
		this.next = next;
	}

	getElemId () {
		return "participantElement_" + this.id;
	}

	makeSelf () {
		this.elem = this.templates.input.cloneNode (true);
		this.elem.id = this.getElemId ();
		this.elem.classList.remove ('template');
		
		this.elem.addEventListener ('input', (ev) => { this.checkParticipantInput (ev)});
		this.elem.addEventListener ('focusout', (ev) => { this.participantInputFoucusOut (ev) });
		this.elem.querySelector ('.removeParticipantIcon').addEventListener ('click', (ev) => { this.removeSelf (ev); });

		this.inp = this.elem.querySelector ('input');
		this.inp.value = '';

		this.templates.input.parentElement.insertBefore (this.elem, this.templates.input);
		this.inp.focus();
	}

	removeSelf (ev) {
		this.elem.remove();
		this.prev?.setNext (this.next);
		this.deletionCallback (this);
	}

	selectItem (item) {
		console.log ('Selected entity #' + item.id);
		this.inp.value = item.title;
		this.selectedId = item.id;
		this.resizeParticipantInput (this.elem.querySelector ('input'));
	}
	
	resizeParticipantInput (elem) {
		let fullWidth = getTextWidth (elem.value, this.elem.querySelector ('input'));
		let mustHaveWidth = Math.max (this.templates.minInputElementWidth, fullWidth);

		let bigbox = document.getElementById ('BigBoxx');
		let maxWidth = bigbox.getBoundingClientRect().width * 0.9;
		if (mustHaveWidth > maxWidth) mustHaveWidth = maxWidth;
		elem.style.width = mustHaveWidth + "px";
	}

	checkParticipantInput (ev) {
		this.selectedId = null;
		this.resizeParticipantInput (ev.target);
		this.suggestionList.suggestFor (ev.target.value);
	}

	participantInputFoucusOut (ev) {
		if (ev.target.value.trim() == "") {
			this.removeSelf (ev);
		} else {
			this.suggestionList.hide();
		}
	}

	getArrayElem () {
		let res = { "created": false, "name": this.inp.value };
		if (this.selectedId != null) {
			res.created = true;
			res.entity_id = this.selectedId;
		}
		return res;
	}
}

class ParticipantList {
	constructor () {
		this.firstParticipant = null;
		this.lastParticipant = null;

	}

	initialize (entity) {
		let inputTemplate = document.getElementById ('participantInputTemplate');
		let suggestionListTemplate = document.getElementById ('participantSuggestionListTemplate');
		let suggestionTemplate = document.getElementById ('participantSuggestionTemplate');

		this.participantTemplates = new ParticipantEntityTemplates (inputTemplate, suggestionListTemplate, suggestionTemplate, 100);

		this.addParticipantButton = document.getElementById ("addParticipantButton");
		this.addParticipantButton.addEventListener ('click', (ev) => { this.addParticipant(); });

		this.container = document.getElementById ('addParticipantField');
		this.entity = entity;
	}

	removeParticipant (part) {
		if (part === this.firstParticipant) {
			this.firstParticipant = part.next;
		}
		if (part === this.lastParticipant) {
			this.lastParticipant = part.prev;
		}
	}

	addParticipant () {
		let prev = (this.lastParticipant == null) ? null : this.lastParticipant;
		let id = (prev == null) ? 0 : (prev.id + 1);
		let newParticipant = new ParticipantEntityInput (prev, id, this.participantTemplates, (ev) => { this.removeParticipant (ev, id) } );
		this.lastParticipant = newParticipant;
		if (this.firstParticipant == null) this.firstParticipant = newParticipant;
	}

	getParticipantsArray () {
		let array = [];
		let iter = this.firstParticipant;
		while (iter != null) {
			array.push (iter.getArrayElem());
			iter = iter.next;
		}

		array.push ({ // Entity on page is considered a participant 
			'created': true,
			'entity_id': parseInt(this.entity.entityData.entity_id),
			'name': this.entity.entityData.name
		});

		return array;
	}

	display () {
		this.container.classList.add ('ondisplay');
	}

	hide () {
		this.container.classList.remove ('ondisplay');
	}
}




/*********************************************************************************************
 *********************************************************************************************
 * 
 *  GENERAL CLASSES
 * 
 *********************************************************************************************
 ********************************************************************************************/


class EventGenerator {
	constructor (entity) {
		this.entity = entity;
		this.inputFactory = new InputFactory (entity);
		this.inputs = [];
		this.currentType = null;
		this.participantList = null;

		this.createEventApiEndpoint = {
			"uri": "/api/events/create",
			"method": "POST"
		};
	}
	
	showEventCreator () {
		this.eventCreatorElem.classList.add ('ondisplay');
		this.addEventButton.classList.remove ('ondisplay');
		this.entity.eventsView.resize();
		this.scrollToView ();
	}

	scrollToView () {
		window.scrollTo ({
			'left': window.scrollX,
			'top': window.scrollY + this.eventCreatorElem.getBoundingClientRect().top,
			'behavior': 'smooth'
		});
	}
	
	hideEventCreator () {
		this.eventCreatorElem.classList.remove ('ondisplay');
		this.addEventButton.classList.add ('ondisplay');
		this.clearInputs ();
		this.reset();

		this.entity.eventsView.resize();
	}
	
	generateOptionsSelector () {
		this.eventGeneratorTypes = [];
		for (let i in this.types) { 
			if (this.types [i].applicable.includes (this.entity.entityData.type) == false) continue;
			this.eventGeneratorTypes.push (new EventGeneratorType (i, this.types[i], this));
		}
	}
	
	clearInputs () {
		for (let i of this.inputs) {
			i.getRootElem ()?.remove();
		}
		this.inputs = [];
	}

	reset () {
		if (this.currentType != null) {
			this.currentType.clear();
			this.currentType = null;
		}
		this.participantList.hide();
	}
	
	addMissingEvent () {
		if (demandAuth()) this.showEventCreator ();
	}
	
	setupEvents () {
		this.addEventButton.addEventListener ('click', () => { this.addMissingEvent(); });
		this.submitButton.addEventListener ('click', () => { this.trySubmit(); });
		this.cancelButton.addEventListener ('click', () => { this.hideEventCreator(); });
	}

	getSubmitData () {
		if (this.currentType == null) return null;
		let eventData = this.currentType.getValues();
		let participants = this.participantList.getParticipantsArray();
		return {"data": eventData, "participants": participants, "type": this.currentType.id};
	}

	async trySubmit () {
		if (this.currentType == null) return;
		if (this.currentType.inputsValid()) {
			let data = this.getSubmitData();
			console.log (data);

			let resp = await fetchApi (this.createEventApiEndpoint, data);
			if (resp == null) {
				flashNetworkError ();
				return;
			}

			console.log (resp);
			if ('error' in resp) {
				flashNetworkError ();
				return;
			} else {
				this.entity.eventsView.constructEvent (resp);
				this.hideEventCreator ();
				this.reset ();
			}
		}
	}

	enableSubmitButton () {
		this.submitButton.classList.remove ('inactive');
	}

	disableSubmitButton () {
		this.submitButton.classList.add ('inactive');
	}

	checkSubmitButtonActivation () {
		if (this.currentType == null) this.disableSubmitButton();
		else {
			if (this.currentType.inputsValid()) {
				this.enableSubmitButton();
			} else {
				this.disableSubmitButton();
			}
		}
	}
	
	displayParticipants () {
		this.participantList.display ();
	}

	initialize () {
		this.types = this.entity.eventTypes;
		this.addEventButton = document.getElementById ('addMissingEvent');
		this.submitButton = document.getElementById ('tryUploadEvent');
		this.cancelButton = document.getElementById ('cancelCreationButton');
		this.eventCreatorElem = document.getElementById ('timelineElemCreate');
		this.timelineTypeSelectorTemplate = document.getElementById ('timelineTypeSelectorTemplate');
		
		this.inputFactory.initialize ();
		this.inputFactory.registerInputType ('text', TextFieldInput);
		this.inputFactory.registerInputType ('current_entity', CurrentEntityInput);
		this.inputFactory.registerInputType ('textarea', TextareaInput);
		this.inputFactory.registerInputType ('date', DateInput);
		this.inputFactory.registerInputType ('band', BandInput);
		
		this.generateOptionsSelector ();
		this.setupEvents ();

		this.participantList = new ParticipantList;
		this.participantList.initialize (this.entity);

		this.submitButton.addEventListener ('mouseenter', () => { this.mouseEnterSubmitButton(); });
		this.submitButton.addEventListener ('mouseleave', () => { this.mouseLeaveSubmitButton(); });
	}
	
	
	mouseEnterSubmitButton (ev) {
		for (let i of this.inputs) {
			if (!i.isValid()) i.getRootElem()?.classList.add ('incorrect');
		}
	}

	mouseLeaveSubmitButton (ev) {
		for (let i of this.inputs) {
			if (!i.isValid()) i.getRootElem()?.classList.remove ('incorrect');
		}
	}
}


class Entity {
	constructor () {
		const params = new URLSearchParams (window.location.search);
		this.id = parseInt (params.get ("id"));

		this.entityDataApiEndpoint = {
			uri: "/api/p",
			method: "POST"
		};
		
		this.getDescriptorsApiEndpoint = {
			uri: "/api/events/types",
			method: "GET"
		}

		this.dataView = new EntityDataView (this);
		this.eventsView = new EventsView (this);
		this.eventGenerator = new EventGenerator (this);
	}

	async getEntityData () {
		let obj = await fetchApi (this.entityDataApiEndpoint, {'id': this.id});
		return obj;
	}

	async getEventTypes () {
		let	fRes = await fetchApi (this.getDescriptorsApiEndpoint);
		if (fRes == null) return null;
		for (let i in fRes ['options']) {
			fRes['options'][i].inputs.sort ((a, b) => { return a.order - b.order; });
		}
		return fRes ['options'];
	}

	unveil () {
		this.veil.classList.add ('nodisplay');
		setTimeout (() => { this.veil.style.display = 'none'}, 200);
	}

	async pullData () {
		this.entityData = await this.getEntityData ();
		this.veil = document.getElementById ('veil');

		if ('redirect' in this.entityData) {
			window.location.replace (this.entityData['redirect']);
		}

		this.eventTypes = await this.getEventTypes ();
		
		console.log (this.entityData);
		console.log (this.eventTypes);

		Promise.all ([
			this.dataView.initialize(),
			this.eventsView.initialize(),
			this.eventGenerator.initialize(),
		]).then (
			() => { this.unveil(); },
			(err) => { flashNetworkError(); console.log (err); } 
		);
	}

}


let localEntity = new Entity;

window.addEventListener (
	'load',
	(ev) => {
		localEntity.pullData ();
	}
);
