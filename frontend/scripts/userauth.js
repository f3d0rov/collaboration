

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

function deviceId () {

}

async function attemptLogin () {
	let usernameElem = document.getElementById ('loginUsername');
	let passwordElem = document.getElementById ('loginPassword');
	
	let username = usernameElem.value;
	let password = passwordElem.value;

	let attempt = await fetch (
		"/api/u/login",
		{
			method: "POST",
			credentials: "same-origin",
			body: JSON.stringify ({"password": password, "username": username})
		}
	);
	
	let response = await attempt.json();
	if (response.status == "success") location.reload();
	else {
		console.log (response);
	}
}

async function logout () {
	await fetch (
		"/api/u/logout",
		{
			method: "POST",
			credentials: "same-origin"
		}
	);

	document.cookie = "username=x; Max-Age=0; SameSite=strict";
	location.reload();
}

async function attemptRegister () {
	let usernameElem = document.getElementById ('regUsername');
	let passwordElem = document.getElementById ('regPassword');
	let emailElem = document.getElementById ('regEmail');

	let username = usernameElem.value;
	let password = passwordElem.value;
	let email = emailElem.value;

	let attempt = await fetch (
		"/api/u/register",
		{
			method: "POST",
			credentials: "same-origin",
			body: JSON.stringify ({"password": password, "username": username, "email": email})
		}
	);
	
	let response = await attempt.json();
	if (response.status == "success") location.reload();
	else {
		console.log (response);
	}
}

function getCookie (name) { // Mozilla's nice code
	return document.cookie.split("; ")
	.find((row) => row.startsWith(name + "="))?.split("=")[1];
}

async function checkAuthOnLoad () {
	let username = getCookie ('username');
	if (username == undefined) return;

	let userCard = document.getElementById ('userCardAuthed');
	let unauthedUserCard = document.getElementById ('userCardUnauthed');

	userCard.classList.remove ('hidden');
	unauthedUserCard.classList.add ('hidden');

	let usernameElem = userCard.querySelector ('#username');
	usernameElem.innerHTML = username;

	let checkAuth = await fetch (
		'/api/u/whoami',
		{
			method: "POST",
			"credentials": "same-origin"
		}
	);
	let obj = await checkAuth.json();

	if (obj.status == "unauthorized") {
		userCard.classList.add ('hidden');
		unauthedUserCard.classList.remove ('hidden');
		document.cookie = "username=null; Max-Age=0";
	} else if (obj.status == "authorized") {
		if (username != obj.username) {
			document.cookie = "username=" + obj.username + "; Max-Age=0; SameSite=strict";
		}
	}
}

function setupLoginForm (ev) {
	let loginButton = document.getElementById ('attemptLogin');
	let registerButton = document.getElementById ('attemptRegister');
	let noAccountButton = document.getElementById ('noAccountButton');
	let haveAccountButton = document.getElementById ('haveAnAccountButton');

	loginButton.addEventListener ('click', attemptLogin);
	registerButton.addEventListener ('click', attemptRegister);
	noAccountButton.addEventListener ('click', showRegisterWindow);
	haveAccountButton.addEventListener ('click', showLoginWindow);

	console.log ("Login form setup");
}

function setupLoginForms (ev) {
	checkAuthOnLoad ();
	let loginButton = document.getElementById ('headerLoginButton');
	let logoutButton = document.getElementById ('logoutButton');
	let registerButton = document.getElementById ('headerRegisterButton');

	loginButton.addEventListener ('click', showLoginWindow);
	logoutButton.addEventListener ('click', logout);
	registerButton.addEventListener ('click', showRegisterWindow);

	setupLoginForm ();
};

window.addEventListener (
	'load',
	setupLoginForms
);