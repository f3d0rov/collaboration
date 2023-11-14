

function getElemBottomCoordsForConnector (elem) {
	let br = elem.getBoundingClientRect ();
	return {
		x: (br.left + br.right) / 2 + window.scrollX,
		y: br.bottom - 16 + window.scrollY
	};
}

function getElemTopCoordsForConnector (elem) {
	let br = elem.getBoundingClientRect ();
	return {
		x: (br.left + br.right) / 2 + window.scrollX,
		y: br.top + 16 + window.scrollY
	};
}

function genConnector (topPoint, bottomPoint) {
	let template = document.getElementById ("timelineConnectorTemplate");
	let clone = template.cloneNode (true);
	line = clone.querySelector ('line');

	clone.classList.remove ('template');
	clone.classList.add ('actualConnector');
	clone.style.left = (topPoint.x - 4) + 'px';
	clone.style.top = topPoint.y + 'px';
	clone.setAttribute ('height', (bottomPoint.y - topPoint.y) + 'px');
	line.setAttribute('y2', bottomPoint.y - topPoint.y);
	template.parentElement.appendChild (clone);

	return clone;
}

function connect (i, a, b) {
	let aIcon = a.querySelector('.timelineElemIcon');
	let bIcon = b.querySelector('.timelineElemIcon');

	let topPoint = getElemBottomCoordsForConnector (aIcon);
	let bottomPoint = getElemTopCoordsForConnector (bIcon);

	let clone = genConnector (topPoint, bottomPoint);
	clone.id = "connector_" + i;
}

function generateConnectors (ev) {
	let nodes = document.querySelectorAll ('.timelineElem');
	let elems = [];

	for (let i of nodes) { // Not connecting templates
		if (!i.classList.contains ('template')) {
			elems.push (i);
		}
	}

	for (let i = 0; i < elems.length - 1; i++) {
		connect (i, elems[i], elems[i + 1]);
	}
}

function reconnect (ev) {
	let connectors = document.querySelectorAll ('.actualConnector');
	generateConnectors ();
	for (let i of connectors) {
		i.remove();
	}
}

function setByClass (elem, classname, innerHtml) {
	let sub = elem.querySelector ('.' + classname);
	sub.innerHTML = innerHtml;
}

function generateLookupUrl (lookupStr) {
	let keys = lookupStr.split (' ');
	if (keys.length == 0) return "https://developer.mozilla.org/en-US/docs/Web/JavaScript";
	let res = "https://www.google.com/search?q=" + keys[0];
	for (let i = 1; i < keys.length; i++) res += "+" + keys[i];
	return res;
}

function editEvent (id) {
	console.log ("edit event #" + id);
}

function reportEvent (id) {
	console.log ("report event #" + id);
}

function generateTimepoint (eventId, icon, titleHtml, date, body = "", lookupStr = "") {
	let template = document.getElementById ('timelineElemTemplate');
	let clone = template.cloneNode (true);
	clone.classList.remove ('template');
	setByClass (clone, 'timepointTitle', titleHtml);
	setByClass (clone, 'timepoint', date);
	setByClass (clone, 'timepointBody', body);
	clone.id = "timepoint_" + eventId;
	clone.querySelector ('.timelineElemIcon').setAttribute ('src', icon);

	if (lookupStr == "") {
		clone.querySelector("#lookupEntry").classList.add ("hidden");	
	} else {
		let url = generateLookupUrl (lookupStr);
		clone.querySelector ('#lookupEntry').addEventListener (
			'click',
			() => {
				window.open (url,'_blank', 'noopener');
			}
		);
	}

	clone.querySelector ('#editEntry').addEventListener (
		'click',
		() => { if (demandAuth()) { editEvent(eventId) } }
	);

	clone.querySelector ('#reportEntry').addEventListener (
		'click',
		() => { if (demandAuth()) { reportEvent(eventId) } }	
	);

	template.parentElement.insertBefore (clone, template);

	return clone;
}

function linkToNameUrlObj (obj) { 
	return '<a href="' + obj.url + '">' + obj.name + '</a>';
}

function generateFoundationTimepoint (event) {
	generateTimepoint (
		event.id,
		'/resources/timeline/band.svg',
		'Основание группы ' + linkToNameUrlObj (event.band),
		event.date,
		'body' in event ? event.body : "",
		"Listen to " + event.band.name
	);
}

function generateSingleTimepoint (event) {
	generateTimepoint (
		event.id,
		'/resources/timeline/song.svg',
		'Сингл ' + linkToNameUrlObj (event.band) + ' - ' + event.song,
		event.date,
		'body' in event ? event.body : "",
		"Listen to " + event.band.name + " - " + event.song
	);
}

function generateAlbumTimepoint (event) {
	generateTimepoint (
		event.id,
		'/resources/timeline/album.svg', 
		'Альбом ' + linkToNameUrlObj (event.band) + ' - ' + linkToNameUrlObj (event.album),
		event.date,
		'body' in event ? event.body : "",
		"Listen to " + event.band.name + " - " + event.album.name
	);
}

function defaultTimepointGenerator (event) {
	// TODO: report error to server
}

function generateEventList (events) {
	const timelineElemGenerators = {
		'band_foundation': generateFoundationTimepoint,
		'single': generateSingleTimepoint,
		'album': generateAlbumTimepoint
	};
	for (let i of events) {
		console.log (i);
		if (i.type in timelineElemGenerators) timelineElemGenerators [i.type](i);
		else defaultTimepointGenerator (i);
	}
	generateConnectors ();
}


class EventCreateForm {
	constructor () {
		// TODO: get this from server
		this.options = {
			"album" : {
				"applicable": new Set([ "person", "band" ]),
				"inputs": [
					{
						"id": "album_name",
						"type": "text",
						"prompt": "Имя альбома"
					}, 
					{
						"id": "description",
						"type": "textarea",
						"prompt": "Описание события",
						"optional": true
					}
				]
			},

			"ep" : {
				"applicable": new Set([ "person", "band" ]),
				"inputs": [
					{
						"id": "single_name",
						"type": "text",
						"prompt": "Имя сингла"
					}, 
					{
						"id": "description",
						"type": "textarea",
						"prompt": "Описание события",
						"optional": true
					}
				]
			},
			
			"band_foundation": {
				"applicable": new Set([ "person" ]),
				"inputs": [
					{
						"id": "band_id",
						"type": "band",
						"prompt": "Группа"
					}, 
					{
						"id": "description",
						"type": "textarea",
						"prompt": "Описание события",
						"optional": true
					}
				]
			},

			"band_join": {
				"applicable": new Set([ "person" ]),
				"inputs": [
					{
						"id": "band_id",
						"type": "band",
						"prompt": "Группа"
					}, 
					{
						"id": "description",
						"type": "textarea",
						"prompt": "Описание события",
						"optional": true
					}
				]
			},

			"band_leave": {
				"applicable": new Set([ "person" ]),
				"inputs": [
					{
						"id": "band_id",
						"type": "band",
						"prompt": "Группа"
					}, 
					{
						"id": "description",
						"type": "textarea",
						"prompt": "Описание события",
						"optional": true
					}
				]
			}
		};

		this.currentInputs = {};
		this.currentOption = 'album';
		this.bandSearchCache = {};
		this.generatePrompt (this.currentOption);
	}

	clearInputs () {
		for (let i in this.currentInputs) {
			this.currentInputs[i].remove();
		}
		this.currentInputs = {};
	}

	setupSuggestionListPosition (list, parent) {
		let box = parent.getBoundingClientRect();
		list.style.top = (box.bottom + window.scrollY) + "px";
		list.style.left = (box.left + window.scrollX) + "px";
		list.style.width = box.width + "px";
	}

	updatePositionOfSuggestions () {
		let all = document.querySelectorAll ('.creationInputSuggestionList');
		for (let i of all) {
			if (i.classList.contains ('template') == false) {
				let elem = document.getElementById (i.id.slice (0, i.id.length - 3)); // Remove "_ss";
				this.setupSuggestionListPosition (i, elem);
			}
		}
	}

	selectOpt (me, parent, opt) {
		let list = document.getElementById (parent.id + "_ss");
		list.remove();
		parent.value = opt.title;

	}

	createSearchSuggestion (optTemplate, elem, opt, index) {
		let clone = optTemplate.cloneNode (true);
		
		clone.innerHTML = opt.title;
		clone.classList.remove ("template");
		clone.id = elem.id + "_ss_" + index;

		clone.addEventListener ('click', (ev) => { this.selectOpt (ev.target, elem, opt); });

		return clone;
	}

	constructBandSearchSuggestions (elem, bands) {
		// TODO
		console.log ("x" + bands.length);
		for (let i of bands) {
			console.log (i.title);
		}

		let oldSuggestions = document.getElementById (elem.id + "_ss");
		let hadOld = oldSuggestions != undefined && oldSuggestions != null;
		let listTemplate = document.getElementById ("creationInputSuggestionListTemplate");
		let optTemplate = document.getElementById ("creationInputSuggestionTemplate");

		let newSuggestions;
		if (hadOld) {
			newSuggestions = oldSuggestions;
			newSuggestions.innerHTML = "";
		} else {
			newSuggestions = listTemplate.cloneNode (true);
			newSuggestions.id = elem.id + "_ss";
			newSuggestions.classList.remove ("template");
			this.setupSuggestionListPosition (newSuggestions, elem);
			listTemplate.parentElement.insertBefore (newSuggestions, listTemplate);
		}

		for (let i = 0; i < bands.length; i++) {
			let clone = this.createSearchSuggestion (optTemplate, elem, bands[i], i);
			newSuggestions.appendChild (clone);
		}

	}

	async checkBandInput (ev) {
		let checkId =  Math.floor (10000000 * Math.random ());
		this.latestCheckId = checkId;
		let value = ev.target.value.trim();

		if (value in this.bandSearchCache) {
			this.constructBandSearchSuggestions (ev.target, this.bandSearchCache[value]);
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
			console.log (res);
			this.bandSearchCache [value] = res.results;
			this.constructBandSearchSuggestions (ev.target, res.results);
		}, /* on rejection */(resp) => {
			if (checkId != this.latestCheckId) return;
		});
	}
	

	generateInput (input) {
		console.log (input);

		let template = document.getElementById ('createTimelineElemInputTemplate');
		let clone = template.cloneNode (true);
		clone.id = input.id + "_cont";
		clone.classList.remove ('template');
		let inputElem = clone.querySelector ('input');
		
		clone.querySelector ('.creationInputLabel').innerHTML = input.prompt;
		
		if (input.type != 'band') {
			if (input.type == 'textarea') {
				let textArea = document.createElement ('textarea');
				textArea.className = inputElem.className;
				inputElem.replaceWith (textArea);
			} else {
				inputElem.setAttribute ('type', input.type);
			}
		} else {
			inputElem.setAttribute ('type', 'text');
			inputElem.addEventListener ('input', (ev) => { this.checkBandInput(ev);} );
		}
		inputElem.id = input.id;
		
		template.parentElement.insertBefore (clone, template);
		this.currentInputs [input.id] = clone;
	}

	generatePrompt (option) {
		if ((option in this.options) == false) return;
		let newOption = this.options [option];
		if (newOption.applicable.has (entityType) == false) return;
		
		this.clearInputs();

		for (let i of newOption.inputs) {
			this.generateInput (i);
		}

		this.currentOption = option;
	}
}

var eventCreateForm = null; // new EventCreateForm;



function getFont (elem) {
	// let inp = this.inputTemplate.querySelector ('input');
	let css = window.getComputedStyle (elem);
	let weight = css ['font-weight'];
	let size = css ['font-size'];
	let family = css ['font-family'];
	let font = `${weight} ${size} ${family}`;
	console.log (font);
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
				console.log (results.results);
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

		let bigbox = document.querySelector ('.timelineElemData');
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
			res.id = this.selectedId;
		}
		return res;
	}
}

class ParticipantList {
	constructor () {
		this.firstParticipant = null;
		this.lastParticipant = null;
		let inputTemplate = document.getElementById ('participantInputTemplate');
		let suggestionListTemplate = document.getElementById ('participantSuggestionListTemplate');
		let suggestionTemplate = document.getElementById ('participantSuggestionTemplate');
		this.participantTemplates = new ParticipantEntityTemplates (inputTemplate, suggestionListTemplate, suggestionTemplate, 100);

		this.addParticipantButton = document.getElementById ("addParticipantButton");
		this.addParticipantButton.addEventListener ('click', (ev) => { this.addParticipant(); })
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
		return array;
	}
}

var participantList = null;

function tryUploadEvent () {
	console.log (participantList?.getParticipantsArray());
}

function switchClicked (ev) {
	let me = ev.target;
	let prev = document.querySelector ('.timelineTypeSelectorButton.selected');
	if (prev != undefined && prev != null) prev.classList.remove ('selected');
	me.classList.add ('selected'); 
	eventCreateForm.generatePrompt (me.getAttribute ('value'));
}

function setupTimelineElementCreator () {
	eventCreateForm = new EventCreateForm;
	participantList = new ParticipantList;
	
	let typeSwitches = document.querySelectorAll ('.timelineTypeSelectorButton');
	for (let i of typeSwitches) {
		i.addEventListener ('click', switchClicked);
	}

	document.getElementById ('tryUploadEvent').addEventListener ('click', tryUploadEvent);
}

window.addEventListener (
	'load', setupTimelineElementCreator
);

window.addEventListener (
	'resize', () => {
		eventCreateForm?.updatePositionOfSuggestions();
	}
)
