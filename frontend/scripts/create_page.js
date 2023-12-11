
class EntityCreator {
	constructor () {
		this.getTypeDescriptorsApi = {
			"uri": "/api/entities/types",
			"method": "GET"
		};
		this.getPictureUploadUrlApi = {
			"uri": "/api/entities/askchangepic",
			"method": "POST"
		};
		this.createEntityApi = {
			"uri": "/api/entities/create",
			"method": "POST"
		};
	}

	async initialize () {
		this.initElements();
		await this.initTypes();
	}

	initElements () {
		this.typeSelectorButtonTemplate = document.getElementById ('typeSelectorButtonTemplate');
		this.imageBox = document.getElementById ("imageBox");
		this.imageElem = document.getElementById ("entityImage");
		this.inputsElem = document.getElementById ("photoAndData");

		this.imageInput =  document.createElement ("input");
		this.imageInput.type = "file";
		this.imageInput.accept = "image/*";

		this.imageBox.addEventListener ('click', () => { this.imageInput.click(); });
		this.imageInput.addEventListener ('change', () => { this.selectImage(); });

		this.nameInputPrompt = document.getElementById ("nameInputPrompt");
		this.nameInputElem = document.getElementById ("name");
		
		let params = new URLSearchParams (window.location.search);
		if (params.has ("q")) {
			this.nameInputElem.value = params.get ("q");
			document.getElementById ('headerSearchBox').value = this.nameInputElem.value;
		}

		this.startDatePrompt = document.getElementById ("startDatePrompt");
		this.startDateElem = document.getElementById ("startDate");

		this.aliveCheckboxCont = document.getElementById ("aliveBox");
		this.aliveCheckbox = document.getElementById ("alive");
		this.aliveCheckbox.addEventListener ('input', () => { this.updateAliveCheckbox(); });

		this.endDatePrompt = document.getElementById ("endDatePrompt");
		this.endDateElem = document.getElementById ("endDate");

		this.descriptionElem = document.getElementById ("description");

		this.createEntityButton = document.getElementById ("createPageButton");
		this.createEntityButton.addEventListener ("click", () => { this.tryCreatePage(); });

		this.createEntityButtonOkIcon = document.getElementById ("okButton");
		this.createEntityButtonLoadingIcon = document.getElementById ("spinnerButton");
	}

	async initTypes () {
		let typeDescriptorsArray = await fetchApi (this.getTypeDescriptorsApi);
		console.log (typeDescriptorsArray.types);
		this.typeDescriptors = {};
		for (let i of typeDescriptorsArray.types) this.initType (i);
	}

	initType (type) {
		this.typeDescriptors [type.type_id] = type;
		this.typeDescriptors [type.type_id].elem = this.createTypeElement (type);
	}

	createTypeElement (type) {
		let clone = cloneTemplate (this.typeSelectorButtonTemplate);
		clone.innerHTML = type.type_name;
		this.typeSelectorButtonTemplate.parentElement.insertBefore (clone, this.typeSelectorButtonTemplate);
		clone.addEventListener ('click', () => { this.selectType (type.type_id); });
		return clone;
	}

	selectType (typeId) {
		this.selectedType = typeId;
		this.type = this.typeDescriptors [typeId];
		this.displaySelectedType (typeId);
		this.applyType (typeId);
	}

	displaySelectedType (typeId) {
		for (let i in this.typeDescriptors) {
			if (i != typeId)
				this.typeDescriptors[i].elem.classList.remove ("selected");
		}
		this.typeDescriptors [typeId].elem.classList.add ("selected");
	}

	applyType (typeId) {
		if (this.type.square_image) {
			this.imageBox.classList.add ("square");
		} else {
			this.imageBox.classList.remove ("square");
		}

		this.nameInputPrompt.innerHTML = escapeHTML (this.type.title_string);
		this.startDatePrompt.innerHTML = escapeHTML (this.type.start_date_string);
		
		if (this.type.has_end_date) {
			this.endDatePrompt.innerHTML = escapeHTML (this.type.end_date_string);
			this.endDateElem.classList.remove ("hidden");
			this.endDatePrompt.classList.remove ("hidden");
			this.aliveCheckboxCont.classList.remove ("hidden");
			this.updateAliveCheckbox();
		} else {
			this.endDateElem.classList.add ("hidden");
			this.endDatePrompt.classList.add ("hidden");
			this.aliveCheckboxCont.classList.add ("hidden");
		}

		this.displayInputs();
	}

	updateAliveCheckbox () {
		if ((!this.aliveCheckbox.checked) && this.type.has_end_date) {
			this.endDateElem.classList.remove ("hidden");
			this.endDatePrompt.classList.remove ("hidden");
		} else {
			this.endDateElem.classList.add ("hidden");
			this.endDatePrompt.classList.add ("hidden");
		}
	}

	displayInputs () {
		this.inputsElem.classList.remove ("invisible");
	}

	selectImage () {
		let file = this.imageInput.files[0];

		if (!this.checkImage (file)) {
			imageSelector.value = "";
			return;
		}
		
		this.imageElem.file = file;
		const reader = new FileReader();
		reader.onload = (e) => {
			this.imageElem.setAttribute('src', e.target.result);
			this.imageElem.classList.remove ('placeholder');
		};

		return new Promise (
			(resolve, reject) => {
				try {
					reader.readAsDataURL (file);
				} catch (ex) { 
					return reject (ex);
				}
				resolve ();
			}
		);
	}

	checkImage (img) {
		// Wrong type
		if (!img.type.startsWith ("image/")) return false;
		// 4 mb - too big
		if (img.size > 4 * 1024 * 1024)	return false;
		return true;
	}

	async tryCreatePage () {
		if (this.lockSubmit === true) return;

		this.lockSubmitButton();
		try {
			let body = this.formCreationBody();
			console.log (body);

			let resp = await fetchApi (this.createEntityApi, body);
			console.log (resp);
			if (resp.status == "success") {
				await this.uploadPicture (resp.entity_id);
				location.href = resp.url;
			} else {
				message (resp.error);
			}
		} catch { /* ??? */}
		this.unlockSubmitButton ();
	}
	
	lockSubmitButton () {
		this.lockSubmit = true;
		this.createEntityButtonLoadingIcon.classList.remove ("template");
		this.createEntityButtonOkIcon.classList.add ("template");
	}

	unlockSubmitButton () {
		this.lockSubmit = false;
		this.createEntityButtonLoadingIcon.classList.add ("template");
		this.createEntityButtonOkIcon.classList.remove ("template");
	}
	
	formCreationBody () {
		let body = {};
		body.name = this.nameInputElem.value;
		body.description = this.descriptionElem.value;
		body.start_date = this.startDateElem.value;
		if ((!this.aliveCheckbox.checked) && this.type.has_end_date)
			body.end_date = this.endDateElem.value;
		body.type = this.type.type_id;
		return body;
	}

	async uploadPicture (entityId) {
		let file = this.imageInput.files[0];
		if (this.checkImage (file) == false) return true;
		let urlReq = await fetchApi (this.getPictureUploadUrlApi, {"entity_id": entityId});
		
		let resp = await fetch (
			urlReq.url,
			{
				"method": "PUT",
				"body": this.imageElem.getAttribute ("src"),
				"credentials": "same-origin"
			}
		);

		if (resp.ok == false)
			message ("Не получилось загрузить изображение!");
		return resp.ok;
	}
};

var entityCreator = new EntityCreator;

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

function setupPage () {
	displayBackgroundOnLoad();
	// setupPageTypeSelector();
	// setupImageInput();
	// setupDataInput();

	entityCreator.initialize();
}

window.addEventListener (
	'load',
	setupPage
);
