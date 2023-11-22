
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

CREATE TABLE events (
	id SERIAL PRIMARY KEY NOT NULL,
	sort_index INT NOT NULL DEFAULT 0,
	type VARCHAR (32) NOT NULL,

	description TEXT
);

create table user_event_contributions (
	id serial primary key not null,
	user_id int references users(uid) ON DELETE SET NULL NOT NULL,
	contributed_on date not null,
	event_id int references events(id) ON DELETE CASCADE
);

CREATE TABLE event_report_reasons (
	id SERIAL PRIMARY KEY,
	name VARCHAR (128) NOT NULL
);

create table event_reports (
	id serial primary key not null,
	event_id int references events(id) ON DELETE CASCADE,
	reported_by int references users(uid) ON DELETE CASCADE,
	reason_id INT REFERENCES event_report_reasons(id) ON DELETE CASCADE NOT NULL,
	UNIQUE (event_id, reported_by, reason_id)
);


CREATE TABLE entities (
	id SERIAL PRIMARY KEY NOT NULL,
	type VARCHAR (16) NOT NULL DEFAULT '',

	name VARCHAR (128) NOT NULL,
	description TEXT NOT NULL DEFAULT '',
	start_date DATE NOT NULL DEFAULT '01-01-1970',
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
	entity_id INT REFERENCES entities(id) ON DELETE CASCADE,
	reported_by INT REFERENCES users(uid) ON DELETE CASCADE,
	reason_id INT NOT NULL,
	UNIQUE (entity_id, reported_by, reason_id)
);


create table personalities (
	id serial primary key not null,
	entity_id INT REFERENCES entities(id) ON DELETE CASCADE
);

create table participation (
	event_id int references events(id) ON DELETE CASCADE,
	entity_id INT REFERENCES entities(id) ON DELETE CASCADE,
	PRIMARY KEY (event_id, entity_id)
);

create table bands (
	id serial primary key not null,
	entity_id INT REFERENCES entities (id) ON DELETE CASCADE
);

CREATE TABLE albums (
	id SERIAL PRIMARY KEY,
	name VARCHAR (256) NOT NULL,
	description TEXT,

	author INT REFERENCES entities (id) ON DELETE SET NULL
);

create table songs (
	id serial primary key not null,
	title varchar (128) not null,

	author INT REFERENCES entities(id) ON DELETE SET NULL,
	album int references albums(id) on delete CASCADE default NULL, -- null for singles
	
	release int references events(id) on delete CASCADE default NULL,
	release_date date
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


CREATE TABLE single_entity_related_events (
	id INT PRIMARY KEY REFERENCES events ON DELETE CASCADE,
	entity_id INT REFERENCES entities NOT NULL,
	event_date DATE NOT NULL
);

INSERT INTO event_report_reasons (name) 
VALUES 
	('Недостоверные данные'),
	('Спам')

