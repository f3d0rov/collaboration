#pragma once

#include "../database.hpp"
#include "../randomizer.hpp"
#include "../mailer.hpp"
#include "../resource.hpp" // UserMistakeException


#define SESSION_ID "session_id"
#define SESSION_ID_LEN 128
#define SALT_LEN 64


// Login mistakes:
#define USER_DOESNT_EXIST_CODE ("user_doesnt_exist")
#define USER_NOT_CONFIRMED_CODE ("user_not_confirmed")
#define PASSWORD_INCORRECT_CODE ("password_incorrect")

// Uniqueness mistakes:
#define EMAIL_TAKEN_CODE ("email_taken")
#define USERNAME_TAKEN_CODE ("name_taken")

// Format mistakes:
#define BAD_EMAIL_CODE ("bad_email")
#define BAD_NAME_CODE ("bad_name")
#define BAD_PASSWORD_CODE ("bad_password")


class UserData;
class UserManager;



class UserData {
	private:
		std::string _username;
		int _id;
		int _accessLevel;
		std::string _lastActionDate;
		std::string _email;
		bool _valid;

	public:
		UserData (bool);
		UserData (int uid, OwnedConnection &conn);
		UserData (UserData &&ud);
		UserData (const UserData &ud);

		int id () const;
		std::string username () const;
		std::string email () const;
		int accessLevel () const;
		std::string lastActionDate () const;
		bool valid () const;
 
		bool hasAccessLevel (int level) const;

		operator int() const; // returns id or -1 if not valid
};


class UserManager {
	private:
		static std::unique_ptr <UserManager> _manager;
		UserManager ();

		std::string generateSalt ();
		std::string hashForPassword (std::string password, std::string salt);
		bool checkPassword (int uid, std::string password, OwnedConnection &conn);

		bool checkPasswordFormat (std::string password);
		bool checkUsernameFormat (std::string username);
		bool checkEmailFormat (std::string email);

		bool emailConfirmed (int userId, OwnedConnection &conn);
		std::string generateSessionId (OwnedConnection& conn);
		std::string authUserWithWork (int uid, std::string device_ip, OwnedConnection &work);

		int getUserIdBySessionId (std::string sessionId, OwnedConnection &conn);
		int getUserIdByUsername (std::string username, OwnedConnection &conn);

		UserData getUserDataWithWork (int uid, OwnedConnection &conn);
		void updateUserLastActionTimeWithWork (int uid, OwnedConnection &conn);

		bool emailIsAvailableWithConn (std::string email, OwnedConnection &conn);
		bool usernameIsAvailableWithConn (std::string username, OwnedConnection &conn);

		void sendConfirmationEmail (std::string username, std::string email, std::string confirmId);

	public:
		UserManager (const UserManager &mgr) = delete;
		UserManager (UserManager &&mgr) = delete;

		static UserManager &get ();

		std::string authUser (int userId, std::string deviceIp);
		std::string loginUser (std::string username, std::string password, std::string deviceIp);
		void logoutUser (std::string sessionId);

		bool emailIsAvailable (std::string email);
		bool usernameIsAvailable (std::string username);
		int registerUser (std::string username, std::string password, std::string email);
		int confirmUserEmail (std::string confirmId);

		UserData getUserDataById (int userId);
		UserData getUserDataBySessionId (std::string sessionId);

		bool userIsAuthorized (std::string sessionId, int requiredAccessLevel = 0);
		static bool isLoggedIn (std::string sessionId, int requiredAccessLevel = 0);

		void updateUserLastActionTime (std::string sessionId);
		void updateUserLastActionTime (int userId);

		bool userExists (int id);
};

