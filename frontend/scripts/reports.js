
class Reporter {
	constructor () {
		this.getReportTypesEndpoint = {
			"uri": "/api/reports/reasons",
			"method": "GET"
		};

		this.reportResourceEndpoint = {
			"uri": "api/reports/report",
			"method": "POST"
		};
	}

	async initialize () {
		this.veil = document.getElementById ("reportWindowVeil");
		this.window = document.getElementById ("reportWindow");
		this.cancelButton = document.getElementById ('cancelReport');
		this.submitButton = document.getElementById ('sendReport');
		this.reportReasonTemplate = document.getElementById ("reportTypeOptionTemplate");

		this.reportReasons = (await fetchApi (this.getReportTypesEndpoint)).reasons;
		this.setupElements ();

		this.selectedReason = null;
		this.currentReasons = [];
	}

	setupElements () {

		this.submitButton.addEventListener ('click', (ev) => { this.submit(); });
		this.cancelButton.addEventListener ('click', () => { this.hideWindow(); });
		this.veil.addEventListener ('click', () => { this.hideWindow(); })

		window.addEventListener ('resize', () => { this.resize(); });
	}

	generateForType (type) {
		let createdClass = "oldReportReasons";
		let oldElems = document.querySelectorAll ('.' + createdClass);
		for (let i of oldElems) {
			i.remove();
		}

		this.currentReasons = [];
		for (let i of this.reportReasons) {
			if (i.reportable_type == type) {
				let clone = this.reportReasonTemplate.cloneNode (true);
				clone.id = "";
				clone.classList.remove ('template');
				clone.classList.add (createdClass);
				clone.innerHTML = escapeHTML(i.name);
				this.reportReasonTemplate.parentElement.insertBefore (clone, this.reportReasonTemplate);
				clone.addEventListener ('click', (ev) => { this.selectReason (ev, i.reason_id); });
				this.currentReasons.push (i);
			}
		}
	}

	enableSubmitButton () {
		this.submitButton.classList.remove ("inactive");
	}

	disableSubmitButton () {
		this.submitButton.classList.add ('inactive');
	}

	selectReason (ev, id) {
		let opponents = document.querySelectorAll ('.selected.reportTypeOption');

		for (let i of opponents) {
			i.classList.remove ('selected');
		}

		ev.target.classList.add ('selected');
		this.selectedReason = id;
		this.enableSubmitButton ();
	}

	resize() {
		let wbcr = this.window.getBoundingClientRect();
		let pbcr = {
			width: window.innerWidth,
			height: window.innerHeight
		};
		this.window.style.left = ((pbcr.width - wbcr.width) / 2) + "px";
		this.window.style.top = ((pbcr.height - wbcr.height) / 2) + "px";
	}
	
	showWindow () {
		this.veil.classList.add ("ondisplay");
		this.window.classList.add ("ondisplay");
		this.veil.classList.remove ("hiddenRep");
		this.window.classList.remove ("hiddenRep");
		this.resize();
	}

	hideWindow () {
		this.veil.classList.add ("hiddenRep");
		this.window.classList.add ("hiddenRep");
		this.selectedId = null;

		setTimeout (() => {
			this.veil.classList.remove ("ondisplay");
			this.window.classList.remove ("ondisplay");
		}, 200);
		this.reset();
	}

	reset () {
		this.selectedReason = null;
		let opponents = document.querySelectorAll ('.selected.reportTypeOption');
		for (let i of opponents) {
			i.classList.remove ('selected');
		}
		this.disableSubmitButton ();
	}

	reportEvent (eventId) {
		this.eventId = eventId;
		this.showWindow ();
	}

	async submit () {
		if (this.selectedReason === null) return;
		let res = fetchApi (this.reportResourceEndpoint, { "ticket": {"reported_id": parseInt(this.selectedId), "reason_id": parseInt(this.selectedReason)}});
		res.then ( () => { message ("Жалоба отправлена!"); });
		this.hideWindow ();
	}

	report (id, type) {
		if (!demandAuth()) return;
		this.selectedId = id;
		this.generateForType (type);
		this.showWindow();
	}
}

var reporter = new Reporter;

window.addEventListener ('load', () => { reporter.initialize(); });
