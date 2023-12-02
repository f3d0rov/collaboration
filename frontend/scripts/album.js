
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
		if (this.elem === null || this.elem === undefined) return;
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
	constructor (prev, id, templates, deletionCallback, partList, value = null) {
		this.list = partList;
		this.prev = prev;
		this.next = null;
		this.prev?.setNext (this);
		
		this.id = id;
		this.templates = templates;
		this.deletionCallback = deletionCallback;
		this.makeSelf(value !== null);

		this.suggestionList = new ParticipantEntitySuggestionList (this);
		this.selectedId = null;

		if (value !== null) {
			this.selectedId = value.entity_id;
			this.inp.value = unescape (value.name);
			this.resizeParticipantInput (this.inp);
		}
	}
	
	setNext (next) {
		this.next = next;
	}

	getElemId () {
		return "participantElement_" + this.id;
	}

	makeSelf (hasValue) {
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
		if (!hasValue)
			this.inp.focus();
	}

	removeSelf (ev) {
		this.elem.remove();
		this.prev?.setNext (this.next);
		this.deletionCallback (this);
		// this.list.eventEditor.resize();
	}

	selectItem (item) {
		console.log ('Selected entity #' + item.id);
		this.inp.value = item.title;
		this.selectedId = item.id;
		this.resizeParticipantInput (this.elem.querySelector ('input'));
	}
	
	resizeParticipantInput (elem, minimize = false) {
		let fullWidth = getTextWidth (elem.value, this.elem.querySelector ('input'));
		let mustHaveWidth = (!minimize) ? 
			Math.max (this.templates.minInputElementWidth, fullWidth)
			: fullWidth;

		let bigbox = this.list.songEditor.elem.querySelector ('.BigBoxx');
		let maxWidth = bigbox.getBoundingClientRect().width * 0.9;
		if (mustHaveWidth > maxWidth) mustHaveWidth = maxWidth;
		elem.style.width = mustHaveWidth + "px";
		// this.list.eventEditor.resize();
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
			setTimeout (
				() => {
					this.resizeParticipantInput (this.inp, true);
				},
				300
			);
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

	initialize (songEditor) {
		this.songEditor = songEditor;

		let inputTemplate = document.getElementById ('participantInputTemplate');
		let suggestionListTemplate = document.getElementById ('participantSuggestionListTemplate');
		let suggestionTemplate = document.getElementById ('participantSuggestionTemplate');

		this.actualEntityInputClass = 'actualEntityInput';
		this.participantTemplates = new ParticipantEntityTemplates (inputTemplate, suggestionListTemplate, suggestionTemplate, 100);

		this.addParticipantButton = this.songEditor.elem.querySelector (
			'.' + this.songEditor.addParticipantButtonClass
		);
		this.addParticipantButton.addEventListener ('click', (ev) => { this.addParticipant(); });

		this.container = this.songEditor.elem.querySelector ('.participants');
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



class EditSongView {
	constructor (aggrView, songData = null) {
		this.songData = songData;
		this.aggrView = aggrView;
		this.next = null;
		
		this.template = document.getElementById ('editTrackTemplate');
		this.participantInputTemplate = document.getElementById ('participantInputTemplate');
		this.suggestionListTemplate = document.getElementById ('participantSuggestionListTemplate');
		this.suggestionItemTemplate = document.getElementById ('participantSuggestionTemplate');

		this.inputClass = 'songInput';
		this.removeParticipantButtonClass = 'removeParticipantIcon';

		this.addParticipantButtonClass = 'addParticipantButton';
		this.participantList = new ParticipantList;

		this.tracklist = document.getElementById ("tracklist");
		this.trackIndexClass = 'trackIndex';

		this.draggedClass = 'dragged';
		this.shadowTrack = document.getElementById ("shadowTrack");
		this.dragged = false;
	}

	construct (songData) {
		this.songData = songData;
		this.constructElement ();
	}

	addParticipant (pe = null) {
		this.participantList.addParticipant (pe);
	}

	constructElement () {
		this.elem?.remove();
		this.elem = cloneTemplate (this.template);

		this.nameInput = this.elem.querySelector ('.' + this.inputClass);
		this.addParticipantButton = this.elem.querySelector ('.' + this.addParticipantButtonClass);

		this.tracklist.insertBefore (this.elem, this.aggrView.next !== null ? this.aggrView.next.elem() : null);
		this.participantList.initialize (this);

		this.trackIndex = this.elem.querySelector ('.' + this.trackIndexClass);
		this.trackIndex.addEventListener ('mousedown', (ev) => { this.startDrag(ev); });
		this.trackIndex.addEventListener ('mouseup', (ev) => { this.endDrag(ev); });
		this.trackIndex.addEventListener ('mousemove', (ev) => { this.moveDrag(ev); });
		this.trackIndex.addEventListener ('mouseleave', (ev) => { this.moveDrag(ev); });
		
		this.elem.addEventListener ('mouseup', (ev) => { this.endDrag(ev); });
		this.elem.addEventListener ('mousemove', (ev) => { this.moveDrag(ev); });
		this.elem.addEventListener ('mouseleave', (ev) => { this.moveDrag(ev); });

		this.trackDragVeil = document.getElementById ("dragTrackVeil");
		
		this.trackDragVeil.addEventListener ('mouseup', (ev) => { this.endDrag(ev); });
		this.trackDragVeil.addEventListener ('mousemove', (ev) => { this.moveDrag(ev); });
		this.trackDragVeil.addEventListener ('mouseleave', (ev) => { this.moveDrag(ev); });

		this.elem.querySelector ('.removeTrack').addEventListener ('click', () => { this.removeTrack(); });

		if (this.songData != null) {
			this.nameInput.value = unescape(this.songData.song);
			this.trackIndex.innerHTML = this.songData.album_index;
			this.album_index = this.songData.album_index;
			this.addParticipant (this.songData.author);
			for (let i of this.songData.participants) {
				if (i.entity_id != this.songData.author.entity_id)
					this.addParticipant (i);
			}
		} else {
			this.addParticipant (this.aggrView.album.albumData.data.author);
		}
		
		this.nameInput.addEventListener ("keydown", (ev) => { if (ev.key == "Enter") this.focusNext();});

	}

	focusNext () {
		if (this.aggrView.next == null) this.aggrView.album.createSong();
		else this.aggrView.next.focus();
	}

	getSongData () {
		let data = {};
		data.data = {};
		data.participants = this.participantList.getParticipantsArray ();
		data.data.song = this.nameInput.value;
		if (this.songData == null) {
			data.id = null;
			data.data.date = this.aggrView.album.albumData.data.date;
			data.data.description = "";
		} else {
			data.id = this.songData.id;
		}
		data.data.album = this.aggrView.album.albumData.id;
		data.data.album_index = this.album_index;
		data.data.author = data.participants.length > 0 ? data.participants[0] : this.aggrView.album.albumData.author; // Debatable
		return data;
	}

	hide () {
		this.elem?.classList.add ("template");
	}

	show () {
		this.elem?.classList.remove ("template");
	}

	focus() {
		this.nameInput.focus();
	}

	startDrag (ev) {
		let bcr = this.elem.getBoundingClientRect ();
		this.dragOrigin = {
			x: bcr.left, //ev.clientX - bcr.left,
			y: ev.clientY - bcr.top
		};

		this.dragged = true;
		this.elem.classList.add (this.draggedClass);

		this.trackDragVeil.classList.remove ("template");
		this.elem.style.width = bcr.width + "px";

		this.shadowTrack.style.width = bcr.width + 'px';
		this.shadowTrack.style.height = bcr.height + 'px';
		this.shadowTrack.classList.remove ('template');

		this.insertAfterSong = null;
	}

	moveDrag (ev) {
		if (this.dragged == false) return;
		let x = ev.pageX;
		let y = ev.pageY;

		// this.elem.style.left = (x - this.dragOrigin.x) + "px";
		this.elem.style.top = (y - this.dragOrigin.y) + "px";
		this.insertAfterSong = this.aggrView.album.placeShadowTrack (this.shadowTrack, this.elem);
	}

	endDrag () {
		if (this.dragged == false) return;
		this.trackDragVeil.classList.add ("template");
		this.elem.classList.remove (this.draggedClass);
		this.elem.style.width = "unset";
		
		this.dragged = false;
		this.shadowTrack.classList.add ('template');

		if (this.insertBeforeSong !== null) {
			this.aggrView.album.placeSongAfter (this.aggrView, this.insertAfterSong);
		}
	}

	removeTrack () {
		this.aggrView.removeSelf();
	}

	setIndex (index) {
		this.album_index = index;
		this.trackIndex.innerHTML = index;
	}
}

class SongView {
	constructor (aggrView, songData) {
		this.songData = songData;
		this.aggrView = aggrView;

		this.template = document.getElementById ("trackTemplate");
		this.tracklist = document.getElementById ("tracklist");

		this.indexClass = 'trackIndex';
		this.nameClass = 'trackName';
		this.authorsClass = 'trackAuthors';

		this.lookupButtonClass = 'lookupEntry';
		this.reportButtonClass = 'reportEntry';

		this.lookupString = null;
	}

	construct (songData) {
		this.songData = songData;
		this.constructElement ();
	}

	hide () {
		this.elem?.classList.add ("template");
	}

	show () {
		this.elem.classList.remove ("template");
	}

	getSongData () {
		return this.songData;
	}

	constructElement () {
		this.elem?.remove();
		this.elem = cloneTemplate (this.template);
		this.elem.querySelector ('.' + this.indexClass).innerHTML = this.songData.album_index;
		this.elem.querySelector ('.' + this.nameClass).innerHTML = this.songData.song;
		this.elem.querySelector ('.' + this.authorsClass).innerHTML = this.generateAuthors();
		this.tracklist.insertBefore (this.elem, this.aggrView.next !== null ? this.aggrView.next.elem() : null);

		this.lookupString = this.songData.song + " - " + this.songData.author.name;
		if ('participants' in this.songData) {
			for (let i of this.songData.participants) {
				if (i.entity_id != this.songData.author.entity_id) {
					this.lookupString += ", " + i.name; 
				}
			}
		}

		this.lookupUrl = getLookupUrl (this.lookupString);

		this.elem.querySelector ('.' + this.lookupButtonClass).addEventListener (
			'click', 
			() => {
				this.lookupThis();
			}
		);

		this.elem.querySelector ('.' + this.reportButtonClass).addEventListener (
			'click',
			() => {
				this.reportThis();
			}
		)
	}

	lookupThis () {
		window.open (this.lookupUrl, '_blank', 'noopener');	
	}

	reportThis () {
		// TODO
	}
	
	generateAuthor (pe) {
		if (pe.created) {
			return '<a href="' + getEntityUrl (pe.entity_id) + '">' + pe.name + "</a>";
		} else {
			return '<a href="' + getUrlForCreation (pe.name) + '">' + pe.name + "</a>";
		}
	}

	generateAuthors () {
		let s = this.generateAuthor (this.songData.author);
		if ('participants' in this.songData) {
			for (let i of this.songData.participants) {
				if (i.entity_id == this.songData.author.entity_id) continue;
				s += ", " + this.generateAuthor (i);
			}
		}
		return s;
	}

	setNext (song) {
		this.next = song;
	}

	index () {
		return this.songData.album_index;
	}

	focus () {
		// NOP
	}

	setIndex (index) {
		this.elem.querySelector ('.' + this.indexClass).innerHTML = index;
	}

	getSongData () {
		return this.songData;
	}
}


class AggregatedSongView {
	constructor (songData, album) {
		this.songData = this.flatten (songData);
		this.album = album;
		if (songData !== null && ('participants' in this.songData == false))
			this.songData.participants = [];
		
		this.basicView = new SongView (this, this.songData);
		this.editView = new EditSongView (this, this.songData);
		this.next = null;
		this.prev = null;

		this.tracklist = document.getElementById ("tracklist");

		this.currentView = songData == null ? this.editView : this.basicView;
		if (this.currentView === this.editView) {
			this.editView.construct (this.songData);
		} else {
			this.basicView.construct (this.songData);
		}
	}

	flatten (songData) {
		if (songData !== null && 'data' in songData) {
			for (let i in songData.data) {
				songData [i] = songData.data[i];
			}
		}
		return songData;
	}

	setNext (song) {
		this.next = song;
		if (song != null) song.prev = this;
	}

	construct () {
		this.currentView.constructElement ();
	}
	
	index () {
		return this.songData.album_index;
	}

	elem () {
		return this.currentView.elem;
	}

	edit () {
		this.editView.construct (this.songData);
		this.basicView.hide();
		// this.tracklist.insertBefore (this.editView.elem, this.next !== null ? this.next.elem() : null);
		this.currentView = this.editView;
		// this.construct();
	}

	view () {
		if (this.songData == null) {
			this.removeSelf();
			return;
		}
		this.basicView.construct (this.songData);
		this.editView.hide();
		// this.tracklist.insertBefore (this.basicView.elem, this.next !== null ? this.next.elem() : null);
		this.currentView = this.basicView;
		this.construct();
	}

	scrollIntoView () {
		window.scroll ({
			"top": this.currentView.elem.getBoundingClientRect().top + window.scrollY,
			"behavior": "smooth"
		});
	}
	
	focus () {
		this.scrollIntoView();
		this.currentView.focus();
	}

	self () {
		return this;
	}

	hide () {
		this.elem().classList.add ("template");
	}
	
	removeSelf () {
		this.elem().remove();
		this.album.removeSong (this);
	}

	setIndex (index) {
		this.currentView.setIndex (index);
	}

	updateSongData (songData) {
		this.songData = songData;
	}

	getSongData () {
		return this.currentView.getSongData ();
	}
};



class AlbumView {
	constructor () {
		this.getAlbumDataApiEndpoint = {
			"uri": "api/albums/get",
			"method": "POST"
		};

		this.updateAlbumDataApiEndpoint = {
			"uri": "api/albums/update",
			"method": "POST"
		};

		this.requestAlbumPictureUploadLink = {
			"uri": "api/album/askchangepic",
			"method": "POST"
		};

		const params = new URLSearchParams (window.location.search);
		this.id = parseInt (params.get ("id"));
	}

	unveil () {
		document.getElementById ("veil").classList.add ("nodisplay");
		setTimeout (() => { document.getElementById ("veil").style.visibility = 'hidden'}, 200);
	}

	async initialize () {
		this.titleElem = document.getElementById ("albumTitle");
		this.authorElem = document.getElementById ("author");
		this.publicationDateElem = document.getElementById ("albumDate");
		this.descriptionElem = document.getElementById ("description");

		this.albumImage = document.getElementById ('albumImage');
		
		this.editAlbumButton = document.getElementById ("editAlbum");
		this.lookupAlbumButton = document.getElementById ("lookupAlbum");
		this.reportAlbumButton = document.getElementById ("reportAlbum");
		
		this.tracklist = document.getElementById ("tracklist");
		this.trackTemplate = document.getElementById ("trackTemplate");
		this.lookupTrackButtonClass = 'lookupEntry';
		this.reportTrackButtonClass = 'reportEntry';

		this.uploadImageOverlay = document.getElementById ("uploadOverlay");

		this.addSongButton = document.getElementById ("addSongButton");
		this.commitChangesButton = document.getElementById ("commitChangesButton");
		this.commitChangesIconOk = document.getElementById ("commitChangesIconOk");
		this.commitChangesIconLoading = document.getElementById ("commitChangesIconLoading");
		this.lockCommitChanges = false;
		this.discardChangesButton = document.getElementById ("discardChangesButton");
		this.editorButtons = document.getElementById ("editButtons");

		this.addSongsPrompt = document.getElementById ("noTracksPrompt");

		this.loadData();

		this.setupEvents();
		this.unveil();
	}

	async loadData () {
		this.albumData = await fetchApi (this.getAlbumDataApiEndpoint, {"id": this.id});
		console.log (this.albumData);

		this.songs = this.albumData.songs;
		this.picture = this.albumData.picture;
		
		this.imgInput = null;

		this.constructInfoCard ();
		this.constructSongs ();
	}

	constructInfoCard () {
		this.titleElem.innerHTML = this.albumData.data.album;
		this.authorElem.innerHTML = '<a href="/e?id=' + this.albumData.data.author.entity_id + '">' + this.albumData.data.author.name + "</a>";
		this.publicationDateElem.innerHTML = dateToString (this.albumData.data.date);
		this.descriptionElem.innerHTML = this.albumData.data.description;
		this.albumImage.setAttribute ("src", this.picture + "?" + new Date().getTime());
	}

	insertSong (song) {
		let prev = null;
		let ptr = this.firstSong;

		if (ptr == null) {
			this.firstSong = song;
			return;
		}

		if (ptr != null && song.index() < ptr.index()) {
			song.setNext (ptr);
			this.firstSong = song;
			return;
		}
		
		do {
			if (song.index() < ptr.index()) {
				prev?.setNext(song);
				song.setNext(ptr);
				return;
			}
			prev = ptr;
			ptr = ptr.next;
		} while (ptr != null);

		// Last
		prev.setNext(song);
	}

	constructSongs () {
		if ('firstSong' in this === false) this.firstSong = null;

		while (this.firstSong !== null) {
			this.firstSong.removeSelf();
			// this.removeSong (this.firstSong);
		}

		if (this.songs.length == 0) {
			this.proposeAddingSongs ();
			this.addSongsPrompt.classList.remove ("template");
			return;
		}
		
		for (let i of this.songs) {
			let song = new AggregatedSongView (i, this);
			this.insertSong (song);
			song.construct ();
		}
	}
	
	createSong () {
		let newSong = new AggregatedSongView (null, this);
		if (this.firstSong == null) {
			this.firstSong = newSong;
		} else {
			let lastSong = this.firstSong;
			while (lastSong.next != null) lastSong = lastSong.next;
			lastSong.setNext(newSong);
		}

		newSong.construct();
		newSong.focus();

		this.recalculateIndices();
	}

	proposeAddingSongs () {

	}

	setupEvents () {
		this.editAlbumButton.addEventListener ('click', () => { this.editAlbum(); });
		this.lookupAlbumButton.addEventListener ('click', () => { this.lookupAlbum(); });
		this.reportAlbumButton.addEventListener ('click', () =>  { this.reportAlbum(); });

		this.addSongButton.addEventListener('click', () => { this.createSong();})
		this.addSongButton.addEventListener('keydown', (ev) => { if (ev.key == "Enter") this.createSong();})

		this.commitChangesButton.addEventListener ('click', () => { this.commitChanges(); });
		this.commitChangesButton.addEventListener ('keydown', (ev) => { if (ev.key == "Enter") this.commitChanges(); });
		
		this.discardChangesButton.addEventListener ('click', () => { this.discardChanges(); });
		this.discardChangesButton.addEventListener ('keydown', (ev) => { if (ev.key == "Enter") this.discardChanges(); });

		this.uploadImageOverlay.addEventListener ('click', () => { this.selectImage(); });

		this.addSongsPrompt.addEventListener ('mouseenter', () => { this.addSongsPromptHoverStart(); });
		this.addSongsPrompt.addEventListener ('mouseleave', () => { this.addSongsPromptHoverEnd(); })
	}

	showEditButtons () {
		this.enableSubmitButton();
		this.editorButtons.classList.remove ("template");
	}
	
	hideEditButtons () {
		this.editorButtons.classList.add ("template");
	}
	
	editAlbum () {
		if (!demandAuth())  return;
		this.addSongsPrompt.classList.add ("template");
		this.editAlbumButton.classList.add ("template");
		this.uploadImageOverlay.classList.remove ("template");
		let song = this.firstSong;
		while (song != null) {
			song.edit ();
			song = song.next;
		}

		this.showEditButtons ();
	}

	viewAlbum () {
		this.editAlbumButton.classList.remove ("template");
		this.uploadImageOverlay.classList.add ("template");
		this.hideEditButtons ();
		
		let song = this.firstSong;
		while (song != null) {
			song.view ();
			song = song.next;
		}
	}

	removeSong (song) {
		if (song === this.firstSong) this.firstSong = song.next;
		if (song.prev !== null) {
			song.prev.setNext (song.next);
		} else if (song.next !== null) {
			song.next.prev = null;
		}
		this.recalculateIndices();
	}


	recalculateIndices () {
		let song = this.firstSong;
		let i = 1;
		while (song !== null) {
			song.setIndex (i);
			i++;
			song = song.next;
		}
	}

	placeSongAfter (song, after) {
		if (after === song) return;
		if (after?.next === song) return;

		if (song === this.firstSong) {
			if (after === null) return;
			this.firstSong = song.next;
			if (this.firstSong !== null) this.firstSong.prev = null;
		}
		song.prev?.setNext (song.next);
		song.prev = null;
		song.next = null;

		if (after !== null) {
			song.setNext (after.next);
			after.setNext (song);
		} else {
			song.setNext (this.firstSong.self());
			this.firstSong = song;
		}

		this.tracklist.insertBefore (song.elem(), song.next !== null ? song.next.elem() : null);
		this.recalculateIndices();
	}

	placeShadowTrack (shadowTrack, draggedElem) {
		let prev = null;
		let placeShadowTrackBefore = this.firstSong;

		let draggedBCR = draggedElem.getBoundingClientRect ();
		let draggedElemMiddle = (draggedBCR.top + draggedBCR.bottom) / 2;

		do {
			if (placeShadowTrackBefore.elem().classList.contains ('dragged') == false) {
				let compBCR = placeShadowTrackBefore.elem().getBoundingClientRect();
				let compMiddle = (compBCR.top + compBCR.bottom) / 2;
	
				if (compMiddle > draggedElemMiddle) {
					this.tracklist.insertBefore (shadowTrack, placeShadowTrackBefore.elem());
					return prev;
				}

				prev = placeShadowTrackBefore;
			}
			placeShadowTrackBefore = placeShadowTrackBefore.next;
		} while (placeShadowTrackBefore != null);

		this.tracklist.appendChild (shadowTrack);
		return prev;
	}

	discardChanges () {
		if (this.lockCommitChanges) return;
		this.constructSongs ();
		this.viewAlbum ();
		this.albumImage.setAttribute ("src", this.picture);
		this.imgInput?.remove();
		this.imgInput = null;
	}

	getSongs () {
		let songs = [];
		let s = this.firstSong;
		while (s !== null) {
			songs.push (s.getSongData());
			s = s.next;
		}
		return songs;
	}

	
	checkImage (file) {
		if (!file.type.startsWith ("image/")) { // Wrong type
			return false;
		}
		if (file.size > 4 * 1024 * 1024) { // 4 mb - too big
			return false;
		}
		return true;
	}

	selectImage () {
		if (this.imgInput === null) {
			this.imgInput = document.createElement ("input");
			this.imgInput.setAttribute ("type", "file");
			this.imgInput.setAttribute ("accept", "image/*");

			this.imgInput.addEventListener (
				'change',
				() => {
					let file = this.imgInput.files[0];
		
					if (!this.checkImage (file)) {
						this.imgInput.value = "";
						return;
					}
					
					this.albumImage.file = file;
					const reader = new FileReader();
					reader.onload = (e) => {
						this.albumImage.setAttribute('src', e.target.result);
						this.albumImage.classList.remove ('placeholder');
					};
					reader.readAsDataURL (file);
				}
			);
		}

		this.imgInput.click();
	}

	disableSubmitButtonForUpload () {
		this.lockCommitChanges = true;
		this.commitChangesIconLoading.classList.remove ("template");
		this.commitChangesIconOk.classList.add ("template");
	}

	enableSubmitButton () {
		this.lockCommitChanges = false;
		this.commitChangesIconLoading.classList.add ("template");
		this.commitChangesIconOk.classList.remove ("template");
	}

	async uploadImage () {
		if (this.imgInput === null) return;

		let file = this.imgInput.files[0];
		if (this.checkImage (file) == false) return;
		
		let reqBody = { id: this.albumData.id };
		let url = (await fetchApi (this.requestAlbumPictureUploadLink, reqBody)).url;

		let resp = await fetch (
			url,
			{
				"method": "PUT",
				"body": this.albumImage.getAttribute ("src"),
				"credentials": 'same-origin'
			}
		);

		if (resp.status == 200) {
			this.imgInput = null;
			this.picture = this.albumImage.getAttribute ("src");
		} else {
			flashNetworkError();
			console.log (resp);
		}

		// this.picture = 
	}

	async commitChanges () {
		if (this.lockCommitChanges) return;

		this.disableSubmitButtonForUpload();
		let songs = this.getSongs ();

		let requestData = {
			id: this.albumData.id,
			songs: songs
		};
		
		await this.uploadImage ();
		
		console.log (requestData);
		let resp = await fetchApi (this.updateAlbumDataApiEndpoint, requestData);
		if (resp === null || resp === undefined) {
			flashNetworkError();
			console.log (resp);
		} else if ('status' in resp) {
			if (resp.status == 'success') {
				await this.loadData();
				this.viewAlbum();
			} else {
				message (resp.status);
				console.log (resp);
			}
		} else if ('error' in resp) {
			message (resp.error);
		} else {
			flashNetworkError();
			console.log (resp);
		}
		
		this.enableSubmitButton();
	}



	lookupAlbum () {
		let lookupStr = this.albumData.data.author.name + " - " + this.albumData.data.album;
		let addr = getLookupUrl (lookupStr);
		window.open (addr, '_blank', 'noopener');
	}

	reportAlbum () {
		if (!demandAuth()) return;
	}
	

	addSongsPromptHoverStart () {
		this.editAlbumButton.classList.add ("beacon");
	}

	addSongsPromptHoverEnd () {
		this.editAlbumButton.classList.remove ("beacon");
	}


}




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

function setupWindow () {
	displayBackgroundOnLoad();
	var album = new AlbumView;
	album.initialize();
}

window.addEventListener (
	'load',
	() => {
		setupWindow();
	}	
);
