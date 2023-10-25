

function hideLoginLayer (ev) {
	let layer = document.getElementById ('loginRegisterLayer');
	if (ev.target == layer) {
		layer.classList.remove ('ondisplay');

		let errors = [
			"loginUsernameError", 
			"loginPasswordError",
			"regUsernameError",
			"regPasswordError",
			"regEmailError"
		];

		for (let i of errors)
			document.getElementById (i).classList.remove ('ondisplay');
	}
}

function showLoginLayer () {
	document.getElementById ('loginRegisterLayer').classList.add ('ondisplay');
}

function showLoginWindow (ev) {
	let loginForm = document.getElementById ('loginForm');
	let regForm = document.getElementById ('registerForm');

	loginForm.classList.add ('ondisplay');
	regForm.classList.remove ('ondisplay');
	showLoginLayer();
	
	placeErrorMessages ();
}

function showRegisterWindow (ev) {
	let loginForm = document.getElementById ('loginForm');
	let regForm = document.getElementById ('registerForm');
	
	loginForm.classList.remove ('ondisplay');
	regForm.classList.add ('ondisplay');
	showLoginLayer();

	placeErrorMessages ();
}

function isAlphanumeric (sym) {
    code = sym.charCodeAt(0);
    if (!(code > 47 && code < 58) && // numeric (0-9)
        !(code > 64 && code < 91) && // upper alpha (A-Z)
        !(code > 96 && code < 123)) { // lower alpha (a-z)
      return false;
    }
	return true;
}

function checkPasswordSanity (pass) {
	return pass.length >= 8;
}

function checkUsernameSanity (username) {
	for (i of username) {
		if (!(isAlphanumeric(i) || "_-.".includes (i))) return false;
	}
	return username.length >= 3;
}

function checkEmailSanity (email) {
	let indexOfAt = email.indexOf ('@');
	return indexOfAt >= 0 && indexOfAt == email.lastIndexOf ('@') && email.lastIndexOf ('@') < email.lastIndexOf('.');
}

function flashInputError (elemId, message = "") {
	const default_msgs = {
		"loginUsernameError": "Пожалуйста, введите корректное имя пользователя.",
		"loginPasswordError": "Пожалуйста, введите корректный пароль.",
		"regUsernameError": 
			"Имя пользователя должно состоять из латинских букв, цифр. Длина - не меньше трех символов. Допустимы точки, дефисы и нижние подчеркивания.",
		"regPasswordError": "В пароле должно быть не меньше 8 символов.",
		"regEmailError": "Введите корректный адрес почты."
	}

	if (message == "") {
		if (elemId in default_msgs) {
			message = default_msgs[elemId];
		} else {
			return;
		}
	}

	let errorElem = document.getElementById (elemId);
	errorElem.innerHTML = message;
	errorElem.classList.add ("ondisplay");
}


async function attemptLogin () {
	let usernameElem = document.getElementById ('loginUsername');
	let passwordElem = document.getElementById ('loginPassword');
	
	let username = usernameElem.value;
	let password = passwordElem.value;


	let fail = false;
	if (!checkUsernameSanity (username)) {
		flashInputError ('loginUsernameError');
		fail = true;
	}
	if (!checkPasswordSanity (password)) {
		flashInputError ('loginPasswordError');
		fail = true;
	}
	if (fail) return;

	let spinner = document.getElementById ('loginSpinner');
	spinner.classList.add ('ondisplay');

	let attempt = await fetch (
		"/api/u/login",
		{
			method: "POST",
			credentials: "same-origin",
			body: JSON.stringify ({"password": password, "username": username})
		}
	);
	
	let response = await attempt.json();
	spinner.classList.remove ('ondisplay');
	switch (response.status) {
		case "success":
			location.reload ();
			return;
		case "no_such_user":
			flashInputError ("loginUsernameError", "Данного пользователя не существует");
			return;
		case "incorrect_password":
			flashInputError ("loginPasswordError", "Неверный пароль");
			return;
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

	let fail = false;	
	if (!checkUsernameSanity (username)) {
		flashInputError ('regUsernameError');
		fail = true;
	}

	if (!checkPasswordSanity (password)) {
		flashInputError ('regPasswordError');
		fail = true;
	}

	if (!checkEmailSanity (email)) {
		flashInputError ('regEmailError');
		fail = true;
	}

	if (fail) return;

	let spinner = document.getElementById ('regSpinner');
	spinner.classList.add ('ondisplay');

	let attempt = await fetch (
		"/api/u/register",
		{
			method: "POST",
			credentials: "same-origin",
			body: JSON.stringify ({"password": password, "username": username, "email": email})
		}
	);
	
	let response = await attempt.json();
	spinner.classList.remove ('ondisplay');

	switch (response.status) {
		case "success":
			location.reload();
			return;
		case "username_taken":
			flashInputError ("regUsernameError", "Выбранное имя занято - выберите другое.");
			return;
		case "email_taken":
			flashInputError ("regEmailError", "Введенный e-mail уже зарегистрирован - возможно, вы хотите <a href=\"#\"> восстановить пароль </a>?");
			return;
		case "incorrect_username_format":
			flashInputError ("regUsernameError");
			return;
		case "incorrect_email_format":
			flashInputError ("regEmailError");
			return;
		case "incorrect_password_format":
			flashInputError ("regPasswordError");
			return;
	}
}

function checkElemInput (elemId, checker) {
	let elem = document.getElementById (elemId);
	let errorElem = document.getElementById (elemId + "Error");
	let val = elem.value;
	if (checker (val)) {
		errorElem.classList.remove ("ondisplay");
	} else {
		flashInputError (elemId + "Error");
		// errorElem.classList.add ("ondisplay");
	}
}

function submitOnEnter (ev, func) {
	if (ev.key == "Enter") { 
		func ();
		ev.preventDefault();
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

function placeErrorMessages () {
	let errors = document.querySelectorAll ('.loginWindowErrorMessage');
	for (let errorElem of errors) {
		let elem = document.getElementById (errorElem.id.replace ("Error", ''));

		let bb = elem.getBoundingClientRect();
		let errBB = errorElem.getBoundingClientRect();

		errorElem.style.left = (bb.right + 40) + "px";
		errorElem.style.top = ((bb.top + bb.bottom) / 2 - errBB.height / 2) + "px";
	}
}

function setupOnClickEvent (elemId, evListener) {
	let elem = document.getElementById (elemId);
	elem.addEventListener ('click', evListener);
}

function setupCheckSanityInputEvent (elemId, checker) {
	let elem = document.getElementById (elemId);
	elem.addEventListener ('input', () => {checkElemInput (elemId, checker)});
}

function setupLoginForm (ev) {
	setupOnClickEvent ('attemptLogin', attemptLogin);
	setupOnClickEvent ('attemptRegister', attemptRegister);
	setupOnClickEvent ('noAccountButton', showRegisterWindow);
	setupOnClickEvent ('haveAnAccountButton', showLoginWindow);
	setupOnClickEvent ('loginRegisterLayer', hideLoginLayer);

	setupCheckSanityInputEvent ('loginUsername', checkUsernameSanity);
	setupCheckSanityInputEvent ('loginPassword', checkPasswordSanity);
	document.getElementById ('loginPassword').addEventListener ('keydown', (ev) => {submitOnEnter (ev, attemptLogin);});
	
	setupCheckSanityInputEvent ('regUsername', checkUsernameSanity);
	setupCheckSanityInputEvent ('regEmail', checkEmailSanity);
	setupCheckSanityInputEvent ('regPassword', checkPasswordSanity);
	document.getElementById ('regPassword').addEventListener ('keydown', (ev) => {submitOnEnter (ev, attemptRegister);});

	window.addEventListener ('resize', placeErrorMessages);
}

function setupLoginForms (ev) {
	checkAuthOnLoad ();

	setupOnClickEvent ('headerLoginButton', showLoginWindow);
	setupOnClickEvent ('headerRegisterButton', showRegisterWindow);
	setupOnClickEvent ('logoutButton', logout);

	setupLoginForm ();
};

window.addEventListener (
	'load',
	setupLoginForms
);
