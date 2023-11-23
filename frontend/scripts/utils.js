
async function fetchApi (endpoint, body = {}) {
	let args = {
		"method": endpoint.method,
		"credentials": 'same-origin'
	};
	if (endpoint.method != "GET") {
		args['body'] = JSON.stringify (body);
	}

	let fRes = await fetch (endpoint.uri, args);

	if (fRes.ok) {
		return await fRes.json();
	} else {
		console.log (await fRes.text());
		return null;
	}
}

function dateToString (date) {
	let abc = date.split ("-");
	return abc[2] + "." + abc[1] + "." + abc[0];
}

function getFont (elem) {
	// let inp = this.inputTemplate.querySelector ('input');
	let css = window.getComputedStyle (elem);
	let weight = css ['font-weight'];
	let size = css ['font-size'];
	let family = css ['font-family'];
	let font = `${weight} ${size} ${family}`;
	return font;
}

var testingCanvas = null;
// with help from https://stackoverflow.com/a/21015393/8665933
function getTextWidth (text, elem) {
	if (testingCanvas == null) {
		testingCanvas = document.createElement ('canvas');
	}
	let ctx = testingCanvas.getContext ('2d');
	ctx.font = getFont (elem);
	return ctx.measureText (text).width;
}

function flashNetworkError () {
	// TODO: this
	console.log ("FlashNetworkError!");
}

function intDateEval (date) {
	let abc = date.split ('-');
	for (let i in abc) abc[i] = parseInt (abc[i]);
	return abc[0] * 10000 + abc[1] * 100 + abc[2];
}

function compareDates (a, b) {
	return intDateEval (a) - intDateEval (b);
}

function getEntityUrl (id){ 
	return '/e?id=' + id;
}

function getUrlForCreation (text) {
	let spl = text.split (" ");
	if (spl.length == 0) return "/create";
	let res = "/create?q=" + spl[0];
	for (let i = 1; i < spl.length; i++) {
		if (spl[i] == "") continue;
		res += "+" + spl[i];	
	}
	return res;
}

function objDiff (orig, changed) {
	let result = {};
	for (let key in changed) {
		if (key in orig == false) {
			result [key] = changed [key];
		} else {
			if (Array.isArray (changed[key]) && Array.isArray (orig[key])) {
				let diff = objDiff (orig [key], changed [key]);
				if (Object.keys (diff).length != 0) result [key] = Object.values (diff);
			} else if (typeof changed [key] == 'object' && typeof orig [key] == 'object') {
				let diff = objDiff (orig [key], changed [key]);
				if (Object.keys (diff).length != 0) result [key] = diff;
			} else {
				if (changed [key] !== orig [key]) result [key] = changed [key];
			}
		}
	}
	return result;
}
