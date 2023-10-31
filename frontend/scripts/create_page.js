

function setupPageTypeSelector () {
	let pageTypes = document.querySelectorAll ('.typeSelectorButton');
	for (let i of pageTypes) {
		i.addEventListener (
			'click',
			(ev) => {
				let button = document.querySelector ('.typeSelectorButton.selected');
				button.classList.remove ('selected');
				ev.target.classList.add ('selected');
			}
		);
	}
}

function setupPage () {
	setupPageTypeSelector();
}

window.addEventListener (
	'load',
	setupPage
);
