

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
	setValue (valid_value) {}
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
		this.promptElem.innerHTML = escapeHTML(this.prompt);

		this.factory.insertInputElem (this.elem);
	}

	getValue () {
		return this.inputElem.value;
	}

	setValue (value) {
		this.inputElem.value = value;
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
		this.baseValue = null;
	}

	getValue () {
		if (this.baseValue === null) {
			return {
				"created": true,
				"name": this.factory.entity.entityData.name,
				"entity_id": parseInt(this.factory.entity.entityData.entity_id)
			};
		}
		return this.baseValue;
	}

	setValue (value) {
		// Set value is called on event edits. It is used here in order to preserve the initial `current_entity` if edit happens on another page
		this.baseValue = value;
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
		this.promptElem.innerHTML = escapeHTML(this.prompt);

		this.factory.insertInputElem (this.elem);
	}

	getValue () {
		return this.inputElem.value;
	}

	setValue (value) {
		this.inputElem.value = value;
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
		this.promptElem.innerHTML = escapeHTML(this.prompt);

		this.inputElem.addEventListener ('resize', () => { this.factory.eventEditor.resize(); });

		this.factory.insertInputElem (this.elem);
	}

	getValue () {
		return this.inputElem.value;
	}

	setValue (value) {
		this.inputElem.value = value;
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
		
		clone.innerHTML = escapeHTML(opt.title);
		clone.classList.remove ("template");
		clone.id = elem.id + "_ss_" + index;

		clone.addEventListener ('click', (ev) => { this.selectOpt (ev.target, elem, opt); });

		return clone;
	}

	constuctList () {
		this.elem?.remove();
		this.elem = this.parent.templates.suggestionList.cloneNode (true);
		this.elem.classList.remove ('template');

		this.parent.elem.parentElement.insertBefore (this.elem, this.parent.elem);
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
		clone.innerHTML = escapeHTML(s.title);
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
		
		this.elem.querySelector (this.templates.labelSelector).innerHTML = escapeHTML(input.prompt);
	
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
		if (ret.created) ret.entity_id = this.selectedItem; 
		return ret;
	}

	setValue (value) {
		this.selectedItem = value.entity_id;
		this.inputElem.value = value.name;
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
	constructor (entity, eventEditor) {
		this.entity = entity;
		this.eventEditor = eventEditor;
		this.inputTypes = {};
	}

	initialize () {
		
	}

	registerInputType (typeId, classRef) {
		if (typeId in this.inputTypes) {
			console.log ("duplicate typeId in InputFactory::registerInputType: '" + typeId + "'");
			throw "duplicate typeId in InputFactory::registerInputType: '" + typeId + "'";
		}

		this.inputTypes [typeId] = classRef;
	}

	insertInputElem (elem) {
		this.eventEditor.inputPointer.parentElement.insertBefore (
			elem,
			this.eventEditor.inputPointer
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
		this.eventGenerator.resize();

		this.eventGenerator.scrollToView ();
	}

	setValues (eventData) {
		let opponents = document.querySelectorAll ('.' + this.primaryClassName + '.' + this.selectedClassName);
		for (let i of opponents) {
			i.classList.remove (this.selectedClassName);
		}

		this.elem.classList.add (this.selectedClassName);
		this.constuctInputs (eventData);
		this.eventGenerator.currentType = this;
		this.eventGenerator.displayParticipants ();
		this.eventGenerator.checkSubmitButtonActivation ();
		this.eventGenerator.resize();

		this.eventGenerator.scrollToView ();
	} 

	constuctInputs (eventData = null) {
		this.eventGenerator.clearInputs ();
		
		for (let i in this.type.inputs) {
			let inp = this.type.inputs[i];
			let newInp = this.inputFactory.construct (inp.type, inp);
			
			newInp.onchange (() => {this.inputCallback();});
			if (eventData !== null && eventData !== undefined) {
				if (inp.id in eventData) {
					newInp.setValue (eventData [inp.id]);
				}
			}
			this.eventGenerator.inputs.push (newInp);
		}
	}

	constructElem () {
		this.elem?.remove();

		this.elem = this.eventGenerator.timelineTypeSelectorTemplate.cloneNode (true);
		this.elem.innerHTML = escapeHTML(this.type.type_display_name);
		this.elem.classList.remove ('template');
		this.elem.id = this.getElemId ();

		this.elem.addEventListener ('click', () => { this.pick(); });

		this.eventGenerator.timelineTypeSelectorRow.appendChild (this.elem);
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
		clone.innerHTML = escapeHTML(text);
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
	constructor (prev, id, templates, deletionCallback, partList, value = null) {
		this.list = partList;
		this.prev = prev;
		this.next = null;
		this.prev?.setNext (this);
		
		this.id = id;
		this.templates = templates;
		this.deletionCallback = deletionCallback;
		this.makeSelf();

		this.suggestionList = new ParticipantEntitySuggestionList (this);
		this.selectedId = null;

		if (value !== null) {
			this.selectedId = value.entity_id;
			this.inp.value = value.name;
			this.resizeParticipantInput (this.inp);
		}
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
		this.elem.classList.add (this.list.actualEntityInputClass);
		
		this.elem.addEventListener ('input', (ev) => { this.checkParticipantInput (ev)});
		this.elem.addEventListener ('focusout', (ev) => { this.participantInputFoucusOut (ev) });
		this.elem.querySelector ('.removeParticipantIcon').addEventListener ('click', (ev) => { this.removeSelf (ev); });

		this.inp = this.elem.querySelector ('input');
		this.inp.value = '';

		this.list.container.insertBefore (this.elem, this.list.addParticipantButton);
		this.inp.focus();
	}

	removeSelf (ev) {
		this.elem.remove();
		this.prev?.setNext (this.next);
		this.deletionCallback (this);
		this.list.eventEditor.resize();
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

		let bigbox = this.list.eventEditor.eventCreatorElem.querySelector ('.BigBoxx');
		let maxWidth = bigbox.getBoundingClientRect().width * 0.9;
		if (mustHaveWidth > maxWidth) mustHaveWidth = maxWidth;
		elem.style.width = mustHaveWidth + "px";
		this.list.eventEditor.resize();
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

	initialize (entity, eventEditor) {
		this.eventEditor = eventEditor;

		let inputTemplate = document.getElementById ('participantInputTemplate');
		let suggestionListTemplate = document.getElementById ('participantSuggestionListTemplate');
		let suggestionTemplate = document.getElementById ('participantSuggestionTemplate');

		this.actualEntityInputClass = 'actualEntityInput';
		this.participantTemplates = new ParticipantEntityTemplates (inputTemplate, suggestionListTemplate, suggestionTemplate, 100);

		this.addParticipantButton = this.eventEditor.eventCreatorElem.querySelector ('.addParticipantButton');
		this.addParticipantButton.addEventListener ('click', (ev) => { this.addParticipant(); });

		this.container = this.eventEditor.eventCreatorElem.querySelector ('.addParticipants');
		this.entity = entity;

		if (this.eventEditor.editedEvent !== null) {
			this.constructItems (this.eventEditor.editedEvent.event.participants);
		}
	}

	constructItems (participants) {
		for (let i of participants) {
			if (i.entity_id == this.eventEditor.entity.entityData.entity_id) continue;
			this.addParticipant (i);
		}
	}

	removeParticipant (part) {
		if (part === this.firstParticipant) {
			this.firstParticipant = part.next;
		}
		if (part === this.lastParticipant) {
			this.lastParticipant = part.prev;
		}
	}

	addParticipant (value) {
		let prev = (this.lastParticipant == null) ? null : this.lastParticipant;
		let id = (prev == null) ? 0 : (prev.id + 1);
		let newParticipant = new ParticipantEntityInput (prev, id, this.participantTemplates, (ev) => { this.removeParticipant (ev, id) }, this, value);
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

	clear () {
		while (this.firstParticipant != null) {
			let next = this.firstParticipant.next;
			this.firstParticipant.elem.remove();
			this.firstParticipant.next = null;
			this.firstParticipant.prev = null;
			this.firstParticipant = next;
		}	
	}
}




/*********************************************************************************************
 *********************************************************************************************
 * 
 *  EVENT GENERATOR (EDITOR)
 * 
 *********************************************************************************************
 ********************************************************************************************/


class EventEditor {
	constructor (entity, editedEvent = null) {
		this.entity = entity;
		this.editedEvent = editedEvent;

		this.inputFactory = new InputFactory (entity, this);
		this.inputs = [];
		this.currentType = null;
		this.participantList = null;

		this.createEventApiEndpoint = {
			"uri": "/api/events/create",
			"method": "POST"
		};

		this.updateEventApiEndpoint = {
			"uri": "/api/events/update",
			"method": "POST"
		};

		if (this.editedEvent !== null) {
			console.log (this.editedEvent);
			this.editedEvent.hide();
		}

		this.template = document.getElementById ('timelineElemCreateTemplate');
		this.createElement ();

		this.types = this.entity.eventTypes;
		this.submitButton = this.eventCreatorElem.querySelector ('.uploadEventButton');
		this.cancelButton = this.eventCreatorElem.querySelector ('.cancelCreation');
		this.timelineTypeSelectorTemplate = document.getElementById ('timelineTypeSelectorTemplate');
		this.timelineTypeSelectorRow = this.eventCreatorElem.querySelector ('.timelineTypeSelector');
		this.inputsContainer = this.eventCreatorElem.querySelector ('.creationInputSuggestionList');
		this.inputPointer = this.eventCreatorElem.querySelector ('.inputPlacer');


		if (this.editedEvent !== null) {
			this.submitButton.innerHTML = "Обновить событие";
		} else {
			this.submitButton.innerHTML = "Создать событие";
		}

		this.participantList = new ParticipantList;
		this.participantList.initialize (this.entity, this);

		this.inputFactory.initialize ();
		this.inputFactory.registerInputType ('text', TextFieldInput);
		this.inputFactory.registerInputType ('current_entity', CurrentEntityInput);
		this.inputFactory.registerInputType ('textarea', TextareaInput);
		this.inputFactory.registerInputType ('date', DateInput);
		this.inputFactory.registerInputType ('band', BandInput);
		
		this.generateOptionsSelector ();
		this.setupEvents ();


		this.submitButton.addEventListener ('mouseenter', () => { this.mouseEnterSubmitButton(); });
		this.submitButton.addEventListener ('mouseleave', () => { this.mouseLeaveSubmitButton(); });

		this.entity.eventsView.resize();
		this.scrollToView();
	}
	
	createElement () {
		let elem = this.template.cloneNode (true);
		elem.id = "";
		elem.classList.remove ('template');
		elem.classList.add ('ondisplay');
		this.eventCreatorElem = elem;

		this.entity.eventsView.root.insertBefore (this.eventCreatorElem, (this.editedEvent === null) ? null : this.editedEvent.getElem());
	}
	

	scrollToView () {
		window.scrollTo ({
			'left': window.scrollX,
			'top': window.scrollY + this.eventCreatorElem.getBoundingClientRect().top - 100,
			'behavior': 'smooth'
		});
	}
	
	hideEventCreator () {
		if (this.editedEvent !== null) {
			this.editedEvent.show();
		}

		this.eventCreatorElem.classList.remove ('ondisplay');
		this.clearInputs ();
		this.reset();

		this.entity.eventsView.resize();
	}
	
	generateOptionsSelector () {
		this.eventGeneratorTypes = [];
		if (this.editedEvent === null) {
			for (let i in this.types) { 
				if (this.types [i].applicable.includes (this.entity.entityData.type) == false) continue;
				this.eventGeneratorTypes.push (new EventGeneratorType (i, this.types[i], this));
			}
		} else {
			this.eventGeneratorTypes.push (new EventGeneratorType (this.editedEvent.event.type, this.types[this.editedEvent.event.type], this));
			this.eventGeneratorTypes [this.eventGeneratorTypes.length - 1].setValues (this.editedEvent.event.data);
		}
		this.resize();
	}
	
	clearInputs () {
		for (let i of this.inputs) {
			i.getRootElem ()?.remove();
		}
		this.inputs = [];
	}

	reset () {
		if (this.editedEvent !== null) {
			this.editedEvent.show();
		}

		if (this.currentType != null) {
			this.currentType.clear();
			this.currentType = null;
		}
		this.participantList.hide();
	}
	
	
	setupEvents () {
		this.submitButton.addEventListener ('click', () => { this.trySubmit(); });
		this.cancelButton.addEventListener ('click', () => { this.done(); });
	}

	getSubmitData () {
		if (this.currentType == null) return null;

		let eventData = this.currentType.getValues();
		let participants = this.participantList.getParticipantsArray();

		if (this.editedEvent !== null) {
			let diff = objDiff (this.editedEvent.event.data, eventData);
			console.log ("diff:");
			console.log (diff);

			let retObj = {
				"participants": participants,
				"type": this.currentType.id,
				"id": this.editedEvent.event.id
			};
			
			if (Object.keys (diff).length != 0) {
				retObj.data = diff;
			}

			return retObj;
		} else {
			return {"data": eventData, "participants": participants, "type": this.currentType.id};
		}
	}

	async trySubmit () {
		if (this.currentType == null) return;
		if (this.currentType.inputsValid()) {
			let data = this.getSubmitData();
			console.log (data);

			let apiEndpoint = (this.editedEvent !== null) ? this.updateEventApiEndpoint : this.createEventApiEndpoint;

			let resp = await fetchApi (apiEndpoint, data);
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
				if (this.editedEvent !== null) {
					message ("Событие изменено!");
				} else {
					message ("Событие создано!");
				}
				this.editedEvent = null; // We replace the old event with the newly constructed one
				this.done ();
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

	clear () {
		if (this.editedEvent !== null) {
			this.editedEvent.show();
		}
		this.participantList = null;
		this.inputFactory = null;
		this.eventCreatorElem.remove();
	}

	done () {
		this.entity.hideEventCreator ();
	}

	resize () {
		this.entity.eventsView.resize ();
	}
}
