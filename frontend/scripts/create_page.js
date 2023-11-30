

async function tryCreatePage (ev) {

	let type = document.querySelector (".typeSelectorButton.selected").getAttribute ("value");
	let name = document.getElementById ("name").value;
	let desc = document.getElementById ("description").value;
	let startDate = document.getElementById ("startDate").value;
	let alive = document.getElementById ("alive").checked;
	let endDate = document.getElementById ("endDate").value;

	let body = {
		"type": type,
		"name": name,
		"description": desc,
		"start_date": startDate
	};
	if (!alive) body["end_date"] = endDate;

	console.log (body);
	let resp = await fetch (
		"/api/create",
		{
			"method": "POST",
			"body": JSON.stringify(body),
			"credentials": "same-origin"
		}
	);

	console.log (resp);
	let respBody = await resp.json ();
	console.log ("" + respBody.status);
	console.log (respBody);
	if (respBody["status"] == "success") {
		let imageSelector = document.getElementById ("imageSelector");
		if (imageSelector.value != 0) await uploadImage(respBody.entity, respBody.upload_url);
		window.location.href = respBody.url;
	}
}

async function uploadImage (entityId, url) {
	let imageSelector = document.getElementById ("imageSelector");
	let file = imageSelector.files[0];
	if (checkImage (file) == false) return;
	let imageBox = document.getElementById ('image');
	
	let resp = await fetch (
		url,
		{
			"method": "PUT",
			"body": imageBox.getAttribute ("src")
		}
	);
	console.log (resp.status);
}

function processTypeSwitch (newType) {
	console.log (newType);

	if (newType == "person") {
		document.getElementById ("image").classList.remove ("square");
		document.getElementById ('startDateName').innerHTML = "Дата рождения";
		
		document.getElementById ('aliveName').innerHTML = "Жив";
		document.getElementById ('aliveBox').classList.remove ('hidden');
		document.getElementById ('alive').dispatchEvent (new Event ('change'));
	} else if (newType == "band") {
		document.getElementById ("image").classList.add ("square");
		document.getElementById ('startDateName').innerHTML = "Дата основания";
		
		document.getElementById ('aliveBox').classList.add ('hidden');
		document.getElementById ('endDateName').classList.add ('hidden');
		document.getElementById ('endDate').classList.add ('hidden');
	} else /* newType == "album" */{
		document.getElementById ("image").classList.add ("square");
		document.getElementById ('startDateName').innerHTML = "Дата выпуска";

		document.getElementById ('aliveBox').classList.add ('hidden');
		document.getElementById ('endDateName').classList.add ('hidden');
		document.getElementById ('endDate').classList.add ('hidden');
	}
}


function setupPageTypeSelector () {
	let pageTypes = document.querySelectorAll ('.typeSelectorButton');
	for (let i of pageTypes) {
		i.addEventListener (
			'click',
			(ev) => {
				let button = document.querySelector ('.typeSelectorButton.selected');
				button.classList.remove ('selected');
				ev.target.classList.add ('selected');
				processTypeSwitch (ev.target.getAttribute('value'));
			}
		);
	}
}

function checkImage (file) {
	if (!file.type.startsWith ("image/")) { // Wrong type
		return false;
	}
	if (file.size > 4 * 1024 * 1024) { // 4 mb - too big
		return false;
	}
	return true;
}

function setupImageInput () { 
	let imageSelector = document.getElementById ('imageSelector');
	imageSelector.addEventListener (
		'change',
		() => {
			let imageSelector = document.getElementById ('imageSelector');
			let file = imageSelector.files[0];

			if (!checkImage (file)) {
				imageSelector.value = "";
				return;
			}
			
			let imageBox = document.getElementById ('image');
			imageBox.file = file;
			const reader = new FileReader();
			reader.onload = (e) => {
				imageBox.setAttribute('src', e.target.result);
				imageBox.classList.remove ('placeholder');
			};
			reader.readAsDataURL (file);
		}
	);

	document.getElementById ('imageBox').addEventListener (
		'click',
		(ev) => {
			let imageSelector = document.getElementById ('imageSelector');
			imageSelector.click();
		}
	)
}

function setupDataInput () {
	let params = new URLSearchParams (window.location.search);
	if (params.has ("q")) {
		document.getElementById ("name").value = params.get ("q");

	}

	let aliveCheckbox = document.getElementById ('alive');
	aliveCheckbox.addEventListener (
		'change',
		() => {
			if (aliveCheckbox.checked) {
				document.getElementById ('endDate').classList.add ('hidden');
				document.getElementById ('endDateName').classList.add ('hidden');
			} else {
				document.getElementById ('endDate').classList.remove ('hidden');
				document.getElementById ('endDateName').classList.remove ('hidden');
			}
		}
	);
	aliveCheckbox.dispatchEvent (new Event('change'));

	document.getElementById ("createPageButton").addEventListener ('click', tryCreatePage);
}

function setupPage () {
	setupPageTypeSelector();
	setupImageInput();
	setupDataInput();
}

window.addEventListener (
	'load',
	setupPage
);
