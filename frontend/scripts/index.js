
function getUrlForSearch (text) {
	let spl = text.split (" ");
	if (spl.length == 0) return "/search";
	let res = "/search?q=" + spl[0];
	for (let i = 1; i < spl.length; i++) {
		if (spl[i] == "") continue;
		res += "+" + spl[i];	
	}
	return res;
}

function search () {
	let prompt = document.getElementById ("searchBox");
	let value = prompt.value;
	if (value == "") {
		let elem = document.getElementById ('searchSuggestionLink');
		value = elem.textContent;
	}
	window.location.href = getUrlForSearch (value);
}

async function displaySearchSuggestion (event) {
	let searchSuggestionFetch = await fetch ('/api/rsp');
	let searchSuggestionObject = await searchSuggestionFetch.json();

	let elem = document.getElementById ('searchSuggestionLink');
	elem.innerHTML = searchSuggestionObject.text;
	elem.href = searchSuggestionObject.url;

	let cont = document.querySelector ('.searchSuggestion');
	cont.style.opacity = '1';
}

function setupSearch () {
	document.getElementById ("searchButton").addEventListener ('click', search);
	let searchInput = document.getElementById ("searchBox");
	searchInput.addEventListener ('keydown', (ev) => { if (ev.key == "Enter") search(); });
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

function setupIndex (ev) {
	displayBackgroundOnLoad();
	displaySearchSuggestion();
	setupSearch();
}

window.addEventListener ('load', setupIndex);
