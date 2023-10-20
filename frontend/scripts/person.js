
function connect (i, a, b) {
	let aIcon = a.querySelector('.timelineElemIcon');
	let bIcon = b.querySelector('.timelineElemIcon');

	let aBR = aIcon.getBoundingClientRect ();
	let bBR = bIcon.getBoundingClientRect ();

	let topPoint = { 
		x: (aBR.left + aBR.right) / 2,
		y: aBR.bottom - 16
	};
	let bottomPoint = {
		x: (bBR.left + bBR.right) / 2,
		y: bBR.top + 16
	};

	let template = document.getElementById ("timelineConnectorTemplate");
	let clone = template.cloneNode (true);
	line = clone.querySelector ('line');

	clone.classList.remove ('template');
	clone.classList.add ('actualConnector');
	clone.style.left = (topPoint.x - 4) + 'px';
	clone.style.top = topPoint.y + 'px';
	line.setAttribute('y2', bottomPoint.y - topPoint.y);
	clone.id = "connector_" + i;

	template.parentElement.appendChild (clone);
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