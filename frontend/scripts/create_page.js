

function processTypeSwitch (newType) {
	console.log (newType);

	if (newType == "person") {
		document.getElementById ("image").classList.remove ("square");
	} else {
		document.getElementById ("image").classList.add ("square");
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
