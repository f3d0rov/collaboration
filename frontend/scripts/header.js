
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
	let prompt = document.getElementById ("headerSearchBox");
	let value = prompt.value;
	if (value == "") {
		value = prompt.getAttribute ("placeholder");
	}
	window.location.href = getUrlForSearch (value);
}

async function setRandomSearchPrompt () {
	let searchSuggestionFetch = await fetch ('/api/rsp');
	let searchSuggestionObject = await searchSuggestionFetch.json();

	let elem = document.getElementById ("headerSearchBox");
	elem.setAttribute ("placeholder", searchSuggestionObject.text);
}

function setupHeader (ev) {
	document.getElementById ("headerSearchButton").addEventListener ('click', search);
	setRandomSearchPrompt();
	let searchInput = document.getElementById ("headerSearchBox");
	searchInput.addEventListener ("keydown", (ev) => { if (ev.key == "Enter") search(); } );

}

window.addEventListener ('load', setupHeader);