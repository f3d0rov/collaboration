

function showLoginLayer () {
	let layer = document.getElementById ('loginRegisterLayer');
	layer.classList.add ('ondisplay');

	layer.addEventListener (
		'click',
		(ev) => {
			if (ev.target == layer) {
				layer.classList.remove ('ondisplay');
			}
		}
	);
}

function showLoginWindow (ev) {
	let loginForm = document.getElementById ('loginForm');
	let regForm = document.getElementById ('registerForm');

	loginForm.classList.add ('ondisplay');
	regForm.classList.remove ('ondisplay');
	showLoginLayer();
}

function showRegisterWindow (ev) {
	let loginForm = document.getElementById ('loginForm');
	let regForm = document.getElementById ('registerForm');

	loginForm.classList.remove ('ondisplay');
	regForm.classList.add ('ondisplay');
	showLoginLayer();
}

function attemptLogin () {
	
}

function attemptRegister () {

}

function setupLoginForm (ev) {
	let loginButton = document.getElementById ('attemptLogin');
	let registerButton = document.getElementById ('attemptRegister');
	let noAccountButton = document.getElementById ('noAccountButton');
	let haveAccountButton = document.getElementById ('haveAnAccountButton');

	loginButton.addEventListener ('click', attemptRegister);
	registerButton.addEventListener ('click', attemptLogin);
	noAccountButton.addEventListener ('click', showRegisterWindow);
	haveAccountButton.addEventListener ('click', showLoginWindow);
}

function setupLoginForms (ev) {
	let loginButton = document.getElementById ('headerLoginButton');
	let registerButton = document.getElementById ('headerRegisterButton');

	loginButton.addEventListener ('click', showLoginWindow);
	registerButton.addEventListener ('click', showRegisterWindow);

	setupLoginForm ();
};

window.addEventListener (
	'load',
	setupLoginForms
);