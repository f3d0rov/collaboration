
var prompt;
var currentPage = 0;
var pageLock = true;

let searchApi = {
	uri: "/api/search",
	method: "POST"
};


function properCountStringForm (count) {
	let res0 = ["найдено", "результатов"];
	let res1 = ["найден", "результат"];
	let res2 = ["найдено", "результата"];
	
	let c100 = count % 100;
	let c10 = c100 % 10;

	if (c100 == 0) return res0; // 0
	if (c100 == 1) return res1; // 1

	if (Math.floor(c100 / 10) == 1) return res0; // 10 - 19
	if (c10 == 1) return res1; // .1
	if (c10 >= 2 && c10 <= 4) return res2; // .2 - .4
	return res0;
}

function countString (count) {
	let properForm = properCountStringForm (count);
	return properForm[0] + " <span class = 'data'>" + count + "</span> " + properForm[1];
}

function cloneResult (result) {
	let template = document.getElementById ('searchResultTemplate');
	let clone = template.cloneNode (true);
	clone.id = "";
	let title = clone.querySelector (".resultTitle");
	title.innerHTML = escapeHTML(result.title);
	title.setAttribute ('href', result.url);
	
	if ('description' in result) {
		let desc = clone.querySelector (".resultDescription");
		desc.classList.add ('ondisplay');
		desc.innerHTML = escapeHTML(result.description);
	}

	if ('picture_url' in result) {
		let img = clone.querySelector (".resultIcon");
		img.setAttribute ("src", result.picture_url);
		img.classList.add ("ondisplay");
		title.classList.add ("major");
		clone.classList.add ("major");
	}

	clone.addEventListener ('click', (ev)=> {window.location.href = result.url;});
	template.parentElement.insertBefore (clone, template);
	clone.classList.remove ("template");
	setTimeout (() => { clone.classList.add ("ondisplay"); }, 10);
}

async function appendSearchResults (results) {
	for (let i of results) {
		cloneResult (i);
		await delay (50);
	}
	// Release page lock
	pageLock = false;
}

async function displaySearchResults (ev) {
	const params = new URLSearchParams (window.location.search);
	prompt = params.get ("q");
	document.getElementById ('headerSearchBox').value = prompt;

	document.getElementById ("createPageResult").addEventListener (
		'click',
		() => {
			if (demandAuth()) {
				window.location.href = "/create?" + params.toString();
			}
		}
	);
	
	let obj = await fetchApi (searchApi, {"prompt": prompt });
	console.log (obj);

	document.getElementById ("searchTime").innerHTML = escapeHTML(obj.time);
	document.getElementById ("resultsFound").innerHTML = countString (obj.total);
	
	if (obj.results.length != 0) {
		appendSearchResults (obj.results);
	}
	pageLock = false;
	document.getElementById ("searchAwait").classList.remove ("ondisplay");
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

async function uploadMoreIfAtEnd () {
	// Return if user didn't scroll to the bottom
	if (Math.abs(window.scrollHeight - window.scrollTop - window.clientHeight) > 5) return;

	// Don't do anything if page is locked. Lock it if it's not.
	if (pageLock) return;
	pageLock = true;
	
	currentPage += 1;
	let resp = await fetchApi (searchApi, {"prompt": prompt, "page": currentPage});
	console.log (resp);
	if (resp.results.length == 0) return; // Don't release `pageLock` if no more search results available
	// Append received search results and release `pageLock`
	appendSearchResults (resp.results);

}

window.addEventListener (
	'load',
	() => {
		displayBackgroundOnLoad ();
		displaySearchResults ();
	}
);

window.addEventListener (
	'scroll',
	() => {
		uploadMoreIfAtEnd ();
	}
);
