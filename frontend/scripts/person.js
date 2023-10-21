
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
	for (let i = 0; i < nodes.length - 1; i++) {
		connect (i, nodes[i], nodes[i + 1]);
	}
}

function reconnect (ev) {
	let connectors = document.querySelectorAll ('.actualConnector');
	generateConnectors ();
	for (let i of connectors) {
		i.remove();
	}
}

window.addEventListener ('load', generateConnectors);
window.addEventListener ('resize', reconnect);