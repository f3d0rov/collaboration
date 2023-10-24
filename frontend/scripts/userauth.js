

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
	return pass.length > 8;
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
		flashInputError ('loginUsername');
		fail = true;
	}
	if (!checkPasswordSanity (password)) {
		flashInputError ('loginPassword');
		fail = true;
	}
	if (fail) return;

	let attempt = await fetch (
		"/api/u/login",
		{
			method: "POST",
			credentials: "same-origin",
			body: JSON.stringify ({"password": password, "username": username})
		}
	);
	
	let response = await attempt.json();
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
		flashInputError ('regUsername');
		fail = true;
	}

	if (!checkPasswordSanity (password)) {
		flashInputError ('regPassword');
		fail = true;
	}

	if (!checkEmailSanity (email)) {
		flashInputError ('regEmail');
		fail = true;
	}

	if (fail) return;

	let attempt = await fetch (
		"/api/u/register",
		{
			method: "POST",
			credentials: "same-origin",
			body: JSON.stringify ({"password": password, "username": username, "email": email})
		}
	);
	
	let response = await attempt.json();

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


function setupLoginForm (ev) {
	let loginButton = document.getElementById ('attemptLogin');
	let registerButton = document.getElementById ('attemptRegister');
	let noAccountButton = document.getElementById ('noAccountButton');
	let haveAccountButton = document.getElementById ('haveAnAccountButton');

	loginButton.addEventListener ('click', attemptLogin);
	registerButton.addEventListener ('click', attemptRegister);
	noAccountButton.addEventListener ('click', showRegisterWindow);
	haveAccountButton.addEventListener ('click', showLoginWindow);

	let loginUsernameInput = document.getElementById ('loginUsername');
	let loginPasswordInput = document.getElementById ('loginPassword');
	let regUsernameInput = document.getElementById ('regUsername');
	let regPasswordInput = document.getElementById ('regPassword');
	let regEmailInput = document.getElementById ('regEmail');

	loginUsernameInput.addEventListener ('input', () => {checkElemInput ('loginUsername', checkUsernameSanity)});
	loginPasswordInput.addEventListener ('input', () => {checkElemInput ('loginPassword', checkPasswordSanity)});
	regUsernameInput.addEventListener ('input', () => {checkElemInput ('regUsername', checkUsernameSanity)});
	regEmailInput.addEventListener ('input', () => {checkElemInput ('regEmail', checkEmailSanity)});
	regPasswordInput.addEventListener ('input', () => {checkElemInput ('regPassword', checkPasswordSanity)});

	window.addEventListener ('resize', placeErrorMessages);
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
