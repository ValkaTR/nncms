-- Database for nncms 0.3

drop table if exists 'users';
drop table if exists 'sessions';
drop table if exists 'acl';
drop table if exists 'log';
drop table if exists 'posts';

create table 'sessions' ( 'id' text, 'user_id' text, 'user_name' text, 'user_agent' text, 'remote_addr' text, 'timestamp' text );
create table 'acl' ( 'id' integer primary key, 'name' text, 'subject' text, 'perm' text );
create table 'users' ( 'id' integer primary key, 'name' text,  'nick' text, 'group' text, 'hash' text );
create table 'log' ( 'id' integer primary key, 'priority' text, 'text' text, 'timestamp' text, 'user_id' text, 'host' text, 'uri' text, 'referer' text, 'user-agent' text, 'src_file' text, 'src_line' text );
create table 'posts' ( 'id' integer primary key, 'user_id' text, 'timestamp' text, 'parent_post_id' text, 'subject' text, 'body' text);
create table 'bans' ( 'id' integer primary key, 'user_id' text, 'timestamp' text, 'ip' text, 'mask' text, 'reason' text);

insert into acl values( null, 'cfg', '@admin', '*' );
insert into acl values( null, 'acl', '@admin', '*' );
insert into acl values( null, 'file_/', '@admin', '*' );
insert into acl values( null, 'user', '@admin', '*' );
insert into acl values( null, 'log', '@admin', '*' );
insert into acl values( null, 'post', '@admin', '*' );

insert into acl values( null, 'user', '@guest', 'register' );
insert into acl values( null, 'user', '@user', 'view' );

insert into acl values( null, 'file_/blocks/navigation.html', '@guest', 'download' );
insert into acl values( null, 'file_/help/', '@guest', 'view' );
insert into acl values( null, 'file_/temp/', '@guest', 'download view' );
insert into acl values( null, 'file_/images/', '@guest', 'download' );
insert into acl values( null, 'file_/templates/', '@guest', 'download' );

insert into acl values( null, 'file_/blocks/navigation.html', '@user', 'download' );
insert into acl values( null, 'file_/help/', '@user', 'view' );
insert into acl values( null, 'file_/temp/', '@user', 'download browse view thumbnail' );
insert into acl values( null, 'file_/images/', '@user', 'download' );
insert into acl values( null, 'file_/templates/', '@user', 'download' );

insert into acl values( null, 'post_1', '@guest', 'view' );
insert into acl values( null, 'post_1', '@user', 'view reply remove modify' );

insert into users values( null, 'guest', 'Anonymous', '@guest', 'nologin' );
insert into users values( null, 'admin', 'Administrator', '@admin', 'c7ad44cbad762a5da0a452f9e854fdc1e0e7a52a38015f23f3eab1d80b931dd472634dfac71cd34ebc35d16ab7fb8a90c81f975113d6c7538dc69dd8de9077ec' );

insert into posts values( null, '1', '1232146914', '0', 'Test subject', 'Test text' );
insert into posts values( null, '1', '1261767564', '1', 'bla', 'blablabla' );
insert into posts values( null, '1', '1261767578', '1', 'xxx', '<i>italic text, <b>bold</b> and <u>underline</u></i>' );
