
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

create table personalities (
	id serial primary key not null,

	ru_name varchar (128) not null,
	native_name varchar (128) not null,

	description text,
	birthday date,
	deathday date,
	picture_path varchar (256),

	awaits_creation boolean default false
);

create table participation (
	event_id int references events(id) ON DELETE CASCADE,
	person_id int references personalities(id) ON DELETE CASCADE
);

create table bands (
	id serial primary key not null,

	name varchar (128) not null,
	description text,
	establishment int references events(id) not null, 
	picture_path varchar (256),

	awaits_creation boolean default false
);

create table albums (
	id serial primary key not null,
	name varchar (128) not null,

	description text,
	release int references events(id) ON DELETE CASCADE NOT NULL,

	awaits_creation boolean default false
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

create table search_index (
	keyword text primary key not null,
	title text not null,
	url text not null,
	value int default 1,
	type text
);
