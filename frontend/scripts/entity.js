


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
		let res = fetchApi (this.reportEventEndpoint, {"event_id": parseInt(this.eventId), "reason_id": parseInt(this.selectedReason)});
		res.then ( () => { message ("Жалоба отправлена!"); });
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
							if (variable.entity_id == this.eventsView.entity.entityData.entity_id) {
								url = "#top";
							} else {
								url = getEntityUrl (variable.entity_id);
							}
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
		if (!demandAuth()) return;
		this.eventsView.eventReporter.reportEvent (this.event.id);
	}

	editEntry () {
		if (!demandAuth()) return;
		this.eventsView.entity.editEvent (this);
	}

	generateParticipant (pc) {
		let clone = this.eventsView.eventParticipantTemplate.cloneNode (true);
		clone.innerHTML = pc.name;
		clone.classList.remove ('template');
		clone.id = '';
		if (pc.created) {
			clone.addEventListener (
				'click',
				() => { 
					this.eventsView.entity.doveil();
					location.href = getEntityUrl (pc.entity_id);
				}
			);
		} else {
			clone.classList.add ('notCreated');
			clone.addEventListener (
				'click',
				() => {
					if (demandAuth()) {
						this.eventsView.entity.doveil();
						location.href = getUrlForCreation (pc.name);
					} 
				}
			);
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

	getElem () {
		return this.elem;
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

	hide () {
		this.elem.classList.add ("template");
		this.eventsView.resize();
	}

	show () {
		this.elem.classList.remove ("template");
		this.eventsView.resize();
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
		this.root = document.getElementById ("timelineContainer");


		this.types = this.entity.eventTypes;
		let events = await this.pullEvents();
		for (let i of events) {
			this.constructEvent (i);
		}

		this.eventReporter.initialize();
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
		this.eventEditor = null;
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
		setTimeout (() => { this.veil.style.visibility = 'hidden'}, 200);
	}

	doveil () {
		this.veil.style.visibility = "visible";
		this.veil.classList.remove ('nodisplay');
		setTimeout (() => { this.unveil()}, 200); // So that going back to this page didn't result in a blank screen
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
			this.eventsView.initialize()
		]).then (
			() => { this.unveil(); },
			(err) => { flashNetworkError(); console.log (err); } 
		);
		
		this.addEventButton = document.getElementById ('addMissingEvent');
		this.addEventButton.addEventListener ('click', () => { this.addMissingEvent(); });
	}

	hideAddEventButton () {
		this.addEventButton.classList.remove ("ondisplay");
	}

	showAddEventButton () {
		this.addEventButton.classList.add ("ondisplay");
	}

	addMissingEvent () {
		if (demandAuth() && this.eventEditor == null) {
			this.hideAddEventButton();
			this.eventEditor = new EventEditor (this, null);
		}
	}
	
	editEvent (event) {
		if (this.eventEditor === null) {
			this.hideAddEventButton();
			this.eventEditor = new EventEditor (this, event);
		}
	}
	
	hideEventCreator () {
		this.addEventButton.classList.add ('ondisplay');
		this.eventEditor.clear ();
		this.eventEditor = null;
		this.showAddEventButton ();
		this.eventsView.resize();
	}
}


let localEntity = new Entity;

function showBackground (img) {
	img.classList.add ("loaded");
}

function displayBackgroundOnLoad () {
	let img = document.querySelector (".sickBackground");
	if (img.complete) {
		showBackground (img);
	} else {
		img.addEventListener ('load', () => { showBackground (img); });
		if (img.complete) showBackground (img); // Just making sure
	}
}

function setBgSrc (src) {
	let img = document.querySelector (".sickBackground");
	img.setAttribute ("src", src);
}

window.addEventListener (
	'load',
	async (ev) => {
		await localEntity.pullData ();
		switch (localEntity.entityData.type) {
			case "person":
				setBgSrc ("/resources/person_bg.jpg");
				break;
			default: // case "band":
				setBgSrc ("/resources/band_bg.jpg");
				break;
		}
		displayBackgroundOnLoad ();
	}
);
