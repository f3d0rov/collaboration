


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
}

function generateSearchResults (results) {
	for (let i of results) {
		cloneResult (i);
	}
}

async function displaySearchResults (ev) {
	const params = new URLSearchParams (window.location.search);
	let prompt = params.get ("q");
	document.getElementById ('headerSearchBox').value = prompt;

	document.getElementById ("createPageResult").addEventListener (
		'click',
		() => {
			if (demandAuth()) {
				window.location.href = "/create?" + params.toString();
			}
		}
	);
	
	let resp = await fetch (
		"/api/search",
		{
			"body": JSON.stringify ({ "prompt": prompt }),
			"method": "POST"
		}
	);
	let obj = await resp.json ();

	console.log (obj);
	for (let i = 0; i < obj.results.length; i++) {
		console.log (obj.results [i]);
	}

	document.getElementById ("searchTime").innerHTML = escapeHTML(obj.time);
	document.getElementById ("resultsFound").innerHTML = countString (obj.total);
	
	if (obj.results.length != 0) {
		generateSearchResults (obj.results);
	}
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

window.addEventListener (
	'load',
	() => {
		displayBackgroundOnLoad ();
		displaySearchResults ();
	}
);
