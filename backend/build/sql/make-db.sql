
CREATE EXTENSION IF NOT EXISTS pg_trgm;

create table users (
	uid serial primary key not null,
	username varchar(64) unique not null,
	email varchar(128) unique,
	
	pass_hash varchar(64),
	pass_salt varchar(64),

	last_access timestamp default CURRENT_TIMESTAMP,
	permission_level int not null default 0
);


CREATE TABLE uploaded_resources (
	id SERIAL PRIMARY KEY,
	path VARCHAR (64) UNIQUE DEFAULT NULL,
	mime VARCHAR (16) DEFAULT NULL,
	uploaded_by INT REFERENCES users(uid) DEFAULT NULL,
	uploaded_on DATE DEFAULT NULL
);


CREATE TABLE resource_upload_links (
	id VARCHAR (128) PRIMARY KEY NOT NULL,
	resource_id INT REFERENCES uploaded_resources (id) NOT NULL,
	valid_until TIMESTAMP NOT NULL
);


CREATE TABLE indexed_resources (
	id SERIAL NOT NULL PRIMARY KEY,
	referenced_id INT,

	url TEXT NOT NULL,
	title TEXT NOT NULL,
	description VARCHAR (256),

	type VARCHAR (32) NOT NULL,
	picture INT REFERENCES uploaded_resources (id) ON DELETE SET NULL,
	CONSTRAINT one_index_per_resource UNIQUE (referenced_id, type)
);

CREATE TABLE search_index (
	resource_id int REFERENCES indexed_resources (id) ON DELETE CASCADE,
	keyword TEXT NOT NULL,
	value INT DEFAULT 1,

	PRIMARY KEY (resource_id, keyword)
);

CREATE INDEX trgm_idx ON search_index USING GIST (keyword gist_trgm_ops);


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
	search_resource INT REFERENCES indexed_resources (id) ON DELETE SET NULL DEFAULT NULL,

	description TEXT
);

create table user_event_contributions (
	id serial primary key not null,
	user_id int references users(uid) ON DELETE SET NULL NOT NULL,
	contributed_on date not null,
	event_id int references events(id) ON DELETE CASCADE
);

CREATE TABLE report_reasons (
	id SERIAL PRIMARY KEY,
	reportable_type VARCHAR (64),
	name VARCHAR (128) NOT NULL
);

CREATE TABLE reports (
	id SERIAL PRIMARY KEY NOT NULL,
	reported_id INT NOT NULL,

	reported_by INT REFERENCES users(uid) ON DELETE CASCADE,
	reason_id INT REFERENCES report_reasons(id) ON DELETE CASCADE NOT NULL,
	reported_on DATE,

	pending BOOL DEFAULT TRUE,
	closed_by INT REFERENCES users(uid) ON DELETE SET NULL DEFAULT NULL,

	CONSTRAINT single_unique_report UNIQUE (reported_id, reported_by, reason_id)
);


CREATE TABLE entities (
	id SERIAL PRIMARY KEY NOT NULL,
	type VARCHAR (16) NOT NULL DEFAULT '',

	name VARCHAR (128) NOT NULL,
	description TEXT NOT NULL DEFAULT '',
	start_date DATE NOT NULL DEFAULT '01-01-1970',
	end_date DATE,
	
	CONSTRAINT start_date_is_past CHECK (start_date <= CURRENT_DATE),
	CONSTRAINT end_date_is_past CHECK (end_date <= CURRENT_DATE),
	CONSTRAINT valid_dates CHECK (start_date <= end_date), -- ? < vs <= - are there any one-day bands?

	search_resource INT REFERENCES indexed_resources (id),

	picture INT REFERENCES uploaded_resources (id) ON DELETE SET NULL,
	awaits_creation BOOLEAN DEFAULT TRUE,
	created_by INT REFERENCES users (uid),
	created_on DATE,
	CHECK (created_on <= CURRENT_DATE)
);

CREATE TABLE entity_photo_upload_links (
	id VARCHAR (128) PRIMARY KEY NOT NULL,
	entity_id INT REFERENCES entities (id),
	valid_until timestamp not null
);

create table participation (
	event_id int references events(id) ON DELETE CASCADE,
	entity_id INT REFERENCES entities(id) ON DELETE CASCADE,
	PRIMARY KEY (event_id, entity_id)
);

CREATE TABLE albums (
	id INT REFERENCES events (id) PRIMARY KEY,
	title VARCHAR (256) NOT NULL,
	author INT REFERENCES entities (id) ON DELETE CASCADE,
	release_date DATE,
	CHECK (release_date <= CURRENT_DATE),
	picture INT REFERENCES uploaded_resources (id) ON DELETE SET NULL
);

CREATE TABLE songs (
	id SERIAL PRIMARY KEY NOT NULL,
	title VARCHAR (128) NOT NULL,

	author INT REFERENCES entities(id) ON DELETE CASCADE,

	album INT REFERENCES albums(id) ON DELETE CASCADE DEFAULT NULL, -- null for singles
	album_index INT DEFAULT 0,
	
	release INT REFERENCES events(id) ON DELETE CASCADE DEFAULT NULL,
	release_date DATE NOT NULL
	CHECK (release_date <= CURRENT_DATE)
);

create table concerts (
	id serial primary key not null,
	name varchar (128) not null,
	picture_path varchar (256),

	event_id int references events(id)
);

CREATE TABLE single_entity_related_events (
	id INT PRIMARY KEY REFERENCES events ON DELETE CASCADE,
	entity_id INT REFERENCES entities NOT NULL,
	event_date DATE NOT NULL,
	CHECK (event_date <= CURRENT_DATE)
);

INSERT INTO report_reasons (reportable_type, name) 
VALUES 
	('entity', 'Недостоверные данные'),
	('entity', 'Некорректное изображение'),
	('entity', 'Спам'),

	('event', 'Недостоверные данные'),
	('event', 'Спам'),

	('album', 'Недостоверные данные'),
	('album', 'Некорректное изображение'),
	('album', 'Спам');


