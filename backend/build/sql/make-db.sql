
create table users (
	uid serial primary key not null,
	username varchar(64) unique not null,
	email varchar(128) unique,
	
	pass_hash varchar(64),
	pass_salt varchar(64),

	permission_level int not null default 0
);

create table user_login (
	session_id varchar (128) primary key not null,
	user_uid int references users (uid) not null,
	last_access timestamp,

	device_id varchar (128)
);

create table user_contributions (
	id serial primary key not null,
	uid int references users(uid) not null,
	contributed_on date not null,
	event_id int -- references events(id)
);

create table events (
	id serial primary key not null,
	event_date date not null,
	event_end_date date,
	event_type varchar (32) not null,

	contribution int references user_contributions (id)
);

create table event_reports (
	id serial primary key not null,
	event_id int references events(id),
	reported_by int references users(uid),
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
	event_id int references events(id),
	person_id int references personalities(id)
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
	release int references events(id) not null,

	awaits_creation boolean default false
);

create table songs (
	id serial primary key not null,
	name varchar (128) not null,

	release int references events(id) default null, -- null for songs in albums
	album int references albums(id) default null, -- null for singles

	band int references bands (id) default null -- null for songs released by personalities
);

create table concerts (
	id serial primary key not null,
	name varchar (128) not null,
	picture_path varchar (256),

	event_id int references events(id)
);

create table search_index (
	keyword text,
	url text,
	count int
);