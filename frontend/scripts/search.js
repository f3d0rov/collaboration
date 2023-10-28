

async function displaySearchResults (ev) {
	const params = new URLSearchParams (window.location.search);
	let prompt = params.get ("q");
	let resp = await fetch (
		"/api/search",
		{
			"body": JSON.stringify ({ "prompt": prompt }),
			"method": "POST"
		}
	);
	let obj = await resp.json ();

	for (let i = 0; i < obj.results.length; i++) {
		console.log (obj.results [i]);
	}
}

window.addEventListener (
	'load',
	displaySearchResults	
);
