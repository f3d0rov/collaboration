
function getElemBottomCoordsForConnector (elem) {
	let br = elem.getBoundingClientRect ();
	return {
		x: (br.left + br.right) / 2 + window.scrollX,
		y: br.bottom - 16 + window.scrollY
	};
}

function getElemTopCoordsForConnector (elem) {
	let br = elem.getBoundingClientRect ();
	return {
		x: (br.left + br.right) / 2 + window.scrollX,
		y: br.top + 16 + window.scrollY
	};
}

function genConnector (topPoint, bottomPoint) {
	let template = document.getElementById ("timelineConnectorTemplate");
	let clone = template.cloneNode (true);
	line = clone.querySelector ('line');

	clone.classList.remove ('template');
	clone.classList.add ('actualConnector');
	clone.style.left = (topPoint.x - 4) + 'px';
	clone.style.top = topPoint.y + 'px';
	clone.setAttribute ('height', (bottomPoint.y - topPoint.y) + 'px');
	line.setAttribute('y2', bottomPoint.y - topPoint.y);
	template.parentElement.appendChild (clone);

	return clone;
}

function connect (i, a, b) {
	let aIcon = a.querySelector('.timelineElemIcon');
	let bIcon = b.querySelector('.timelineElemIcon');

	let topPoint = getElemBottomCoordsForConnector (aIcon);
	let bottomPoint = getElemTopCoordsForConnector (bIcon);

	let clone = genConnector (topPoint, bottomPoint);
	clone.id = "connector_" + i;
}

function generateConnectors (ev) {
	let nodes = document.querySelectorAll ('.timelineElem');
	let elems = [];

	for (let i of nodes) { // Not connecting templates
		if (!i.classList.contains ('template')) {
			elems.push (i);
		}
	}

	for (let i = 0; i < elems.length - 1; i++) {
		connect (i, elems[i], elems[i + 1]);
	}
}

function reconnect (ev) {
	let connectors = document.querySelectorAll ('.actualConnector');
	generateConnectors ();
	for (let i of connectors) {
		i.remove();
	}
}

function setByClass (elem, classname, innerHtml) {
	let sub = elem.querySelector ('.' + classname);
	sub.innerHTML = innerHtml;
}

function generateTimepoint (icon, titleHtml, date, body = "") {
	let template = document.getElementById ('timelineElemTemplate');
	let clone = template.cloneNode (true);
	clone.classList.remove ('template');
	setByClass (clone, 'timepointTitle', titleHtml);
	setByClass (clone, 'timepoint', date);
	setByClass (clone, 'timepointBody', body);
	clone.querySelector ('.timelineElemIcon').setAttribute ('src', icon);

	template.parentElement.appendChild (clone);

	return clone;
}

function linkToNameUrlObj (obj) { 
	return '<a href="' + obj.url + '">' + obj.name + '</a>';
}

function generateFoundationTimepoint (event) {
	generateTimepoint (
		'/resources/timeline/band.svg',
		'Основание группы ' + linkToNameUrlObj (event.band),
		event.date,
		'body' in event ? event.body : ""
	);
}

function generateSingleTimepoint (event) {
	generateTimepoint (
		'/resources/timeline/song.svg',
		'Сингл ' + linkToNameUrlObj (event.band) + ' - ' + event.song,
		event.date,
		'body' in event ? event.body : ""
	);
}

function generateAlbumTimepoint (event) {
	generateTimepoint (
		'/resources/timeline/album.svg', 
		'Альбом ' + linkToNameUrlObj (event.band) + ' - ' + linkToNameUrlObj (event.album),
		event.date,
		'body' in event ? event.body : ""
	);
}

function defaultTimepointGenerator (event) {
	// TODO: report error to server
}

function generateEventList (events) {
	const timelineElemGenerators = {
		'band_foundation': generateFoundationTimepoint,
		'single': generateSingleTimepoint,
		'album': generateAlbumTimepoint
	};
	for (let i of events) {
		if (i.type in timelineElemGenerators) timelineElemGenerators [i.type](i);
		else defaultTimepointGenerator (i);
	}
	generateConnectors ();
}

var testData = [
	{
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

function generatePage (ev) {
	// TODO: pull data from API
	generateEventList (testData);

}

window.addEventListener ('load', generatePage);
window.addEventListener ('resize', reconnect);