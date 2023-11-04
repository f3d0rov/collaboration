
create table users (
	uid serial primary key not null,
	username varchar(64) unique not null,
	email varchar(128) unique,
	
	pass_hash varchar(64),
	pass_salt varchar(64),

	last_access timestamp default CURRENT_TIMESTAMP,
	permission_level int not null default 0
);

create table pending_email_confirmation (
	uid int references users (uid) on delete cascade not null,
	confirmation_id varchar (128) primary key not null,
	valid_until timestamp not null
);

create table user_login (
	session_id varchar (128) primary key not null,
	user_uid int references users (uid) ON DELETE CASCADE NOT NULL,
	last_access timestamp,

	device_ip inet
);

create table user_contributions (
	id serial primary key not null,
	uid int references users(uid) ON DELETE SET NULL NOT NULL,
	contributed_on date not null,
	event_id int -- references events(id)
);

create table events (
	id serial primary key not null,
	event_date date not null,
	event_end_date date,
	event_type varchar (32) not null,

	contribution int references user_contributions (id) ON DELETE SET NULL
);

create table event_reports (
	id serial primary key not null,
	event_id int references events(id) ON DELETE CASCADE,
	reported_by int references users(uid) ON DELETE CASCADE,
	reason_id int not null
);

CREATE TABLE entities (
	id SERIAL PRIMARY KEY NOT NULL,
	type VARCHAR (16) NOT NULL,

	name VARCHAR (128) NOT NULL,
	description TEXT NOT NULL,
	start_date DATE NOT NULL,
	end_date DATE,

	picture_path VARCHAR (256),
	awaits_creation BOOLEAN DEFAULT TRUE,
	created_by INT REFERENCES users (uid),
	created_on DATE
);

CREATE TABLE entity_photo_upload_links (
	id VARCHAR (128) PRIMARY KEY NOT NULL,
	entity_id INT REFERENCES entities (id),
	valid_until timestamp not null
);

CREATE TABLE entity_reports (
	id SERIAL PRIMARY KEY NOT NULL,
	event_id INT REFERENCES events(id) ON DELETE CASCADE,
	reported_by INT REFERENCES users(uid) ON DELETE CASCADE,
	reason_id INT NOT NULL	
);


create table personalities (
	id serial primary key not null,
	entity_id INT REFERENCES entities(id) ON DELETE CASCADE
);

create table participation (
	event_id int references events(id) ON DELETE CASCADE,
	person_id INT REFERENCES personalities(id) ON DELETE CASCADE
);

create table bands (
	id serial primary key not null,
	entity_id INT REFERENCES entities (id) ON DELETE CASCADE
);

create table albums (
	id serial primary key not null,
	entity_id INT REFERENCES entities (id) ON DELETE CASCADE
);

create table songs (
	id serial primary key not null,
	name varchar (128) not null,

	release int references events(id) on delete SET NULL default NULL, -- null for songs in albums
	album int references albums(id) on delete CASCADE default NULL, -- null for singles

	band int references bands (id) on delete SET NULL DEFAULT NULL -- null for songs released by personalities
);

create table concerts (
	id serial primary key not null,
	name varchar (128) not null,
	picture_path varchar (256),

	event_id int references events(id)
);

CREATE TABLE indexed_resources (
	id SERIAL NOT NULL PRIMARY KEY,
	url TEXT NOT NULL UNIQUE,
	title TEXT NOT NULL,
	description TEXT,
	type TEXT NOT NULL,
	picture_path TEXT
);

CREATE TABLE search_index (
	resource_id int REFERENCES indexed_resources (id) ON DELETE CASCADE,
	keyword TEXT NOT NULL,
	value INT DEFAULT 1
);


SELECT url, title, type, picture_path, SUM(value)
FROM indexed_resources INNER JOIN search_index ON indexed_resources.id = search_index.resource_id
WHERE keyword IN ('trent') GROUP BY url, title, type, picture_path ORDER BY SUM(value);
