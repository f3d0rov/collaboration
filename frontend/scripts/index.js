
async function displaySearchSuggestion (event) {
	let searchSuggestionFetch = await fetch ('/api/rsp');
	let searchSuggestionObject = await searchSuggestionFetch.json();

	let elem = document.getElementById ('searchSuggestionLink');
	elem.innerHTML = searchSuggestionObject.text;
	elem.href = searchSuggestionObject.url;

	let cont = document.querySelector ('.searchSuggestion');
	cont.style.opacity = '1';
}

window.addEventListener ('load', displaySearchSuggestion);
