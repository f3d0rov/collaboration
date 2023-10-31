

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

function setupImageInput () { 
	document.getElementById ('imageBox').addEventListener (
		'click',
		(ev) => {
			document.getElementById ('imageSelector').click();
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
