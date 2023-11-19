
var testData = [
	{
		id: 20,
		type: 'band_foundation',
		band: { 
			name: 'Nine Inch Nails',
			url: '/b?=12'
		},
		date: '1988',
		collaborators: [],
		body: "В 1994 году Резнор сказал в интервью, что выбрал для группы название «Nine Inch Nails», потому что оно хорошо звучало и прошло проверку временем (большинство названий спустя какое-то время начинали звучать плохо), а также легко превращалось в аббревиатуру."
	}, 

	{
		id: 21,
		type: 'single',
		band: { 
			name: 'Nine Inch Nails',
			url: '/b?=12'
		},
		song: "Down In It",
		date: '15.09.1988',
		collaborators: []
	},

	{
		id: 22,
		type: 'album',
		band: { 
			name: 'Nine Inch Nails',
			url: '/b?=12'
		},
		album: {
			name: 'Pretty Hate Machine',
			url: '/a?=12'
		},

		date: '20.10.1988',
		collaborators: []
	},
	
	{
		id: 23,
		type: 'single',
		band: { 
			name: 'Nine Inch Nails',
			url: '/b?=12'
		},
		song: "Head Like A Hole",
		date: '22.03.1989',
		collaborators: []
	},
	{
		id: 36,
		type: 'single',
		band: { 
			name: 'Nine Inch Nails',
			url: '/b?=12'
		},
		song: 'Sin',
		date: '10.10.1990',
		collaborators: []
	}
];

function generate404page () {
	console.log (404);
}

async function pullPersonData () {
	const params = new URLSearchParams (window.location.search);
	let id = params.get ("id");
	let apiEndpoint = "/api/p";
	let apiReqBody = {
		"id": parseInt(id)
	};
	
	let req = await fetch (
		apiEndpoint,
		{
			"method": "POST",
			"body": JSON.stringify(apiReqBody)
		}
	);

	if (req.status == 404) {
		generate404page();
	} else {
		let resp = await req.json ();
		document.getElementById ("personName").innerHTML = resp.name;
		document.getElementById ("personLifetime").innerHTML = 
			"(" + dateToString(resp.start_date) + ('end_date' in resp ? " - " + dateToString(resp.end_date) : "") + ")";
		document.getElementById ("personBio").innerHTML = resp.description;
		if ("picture_path" in resp) document.getElementById ("personImage").setAttribute ("src", resp.picture_path);
	}
}

function dateToString (date) {
}

function generatePage (ev) {
	pullPersonData ();
	// TODO: pull event data from API
	generateEventList (testData);

}


window.addEventListener ('load', generatePage);
window.addEventListener ('resize', reconnect);
