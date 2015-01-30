-- Database for nncms 0.3

-- (11) How do I add or delete columns from an existing table in SQLite.
--
-- SQLite has limited ALTER TABLE support that you can use to add a
-- column to the end of a table or to change the name of a table. If
-- you want to make more complex changes in the structure of a table,
-- you will have to recreate the table. You can save existing data to a
-- temporary table, drop the old table, create the new table, then copy
-- the data back in from the temporary table.
--
-- For example, suppose you have a table named "t1" with columns names
-- "a", "b", and "c" and that you want to delete column "c" from this
-- table. The following steps illustrate how this could be done:
--
-- BEGIN TRANSACTION;
-- CREATE TEMPORARY TABLE field_config_backup( 'id' integer primary key, 'node_type_id' text, 'name' text, 'type' text, 'config' text );
-- INSERT INTO field_config_backup SELECT * FROM field_config;
-- DROP TABLE field_config;
-- CREATE TABLE t1(a,b);
-- INSERT INTO field_config SELECT * FROM field_config_backup;
-- DROP TABLE field_config_backup;
-- COMMIT;
--
-- SELECT * FROM sqlite_master;
--
-- PRAGMA table_info(sqlite_master);
--
-- ALTER TABLE table_name ADD column_name datatype;

drop table if exists 'sessions';
drop table if exists 'acl';
drop table if exists 'users';
drop table if exists 'log';
drop table if exists 'posts';
drop table if exists 'bans';
drop table if exists 'locales_source';
drop table if exists 'locales_target';
drop table if exists 'languages';
drop table if exists 'node';
drop table if exists 'node_rev';
drop table if exists 'node_type';
drop table if exists 'field_config';
drop table if exists 'field_data';
drop table if exists 'files';
drop table if exists 'xxx';
drop table if exists 'views';
drop table if exists 'views_filter';
drop table if exists 'views_sort';
drop table if exists 'field_view';

create table 'sessions' ( 'id' text, 'user_id' integer, 'user_name' text, 'user_agent' text, 'remote_addr' text, 'timestamp' integer );
create table 'acl' ( 'id' integer primary key, 'name' text, 'subject' text, 'perm' text, 'grant' text );
create table 'users' ( 'id' integer primary key, 'name' text,  'nick' text, 'group' text, 'hash' text, 'avatar' text, 'folder_id' text );
create table 'groups' ( 'id' integer primary key, 'name' text,  'description' text );
create table 'usergroups' ( 'id' integer primary key, 'user_id' text,  'group_id' text );
create table 'log' ( 'id' integer primary key, 'priority' text, 'text' text, 'timestamp' integer, 'user_id' integer, 'user_host' text, 'user_agent' text, 'uri' text, 'referer' text, 'function' text, 'src_file' text, 'src_line' text );
create table 'posts' ( 'id' integer primary key, 'user_id' integer, 'timestamp' integer, 'parent_post_id' integer, 'subject' text, 'body' text, 'type' text, 'group_id' integer, 'mode' text);
create table 'bans' ( 'id' integer primary key, 'user_id' integer, 'timestamp' integer, 'ip' text, 'mask' text, 'reason' text);
create table 'locales_source' ( 'id' integer primary key, 'location' text, 'textgroup' text, 'source' text);
create table 'locales_target' ( 'id' integer primary key, 'lid' integer, 'language' text, 'translation' text);
create table 'languages' ( 'id' integer primary key, 'name' text, 'native' text, 'prefix' text, 'weight' integer, 'enabled' integer);
create table 'node' ( 'id' integer primary key, 'user_id' integer, 'node_type_id' integer, 'language' text, 'node_rev_id' integer, 'published' integer, 'promoted' integer, 'sticky' integer );
create table 'node_rev' ( 'id' integer primary key, 'node_id' integer, 'user_id' integer, 'title' text, 'timestamp' integer, 'log_msg' text );
create table 'node_type' ( 'id' integer primary key, 'name' text, 'authoring' integer, 'comments' integer );
create table 'field_config' ( 'id' integer primary key, 'node_type_id' integer, 'name' text, 'type' text, 'config' text, 'weight' integer, 'label' text, 'empty_hide' integer, 'values_count' integer, 'size_min' integer, 'size_max' integer, 'char_table' text );
create table 'field_data' ( 'id' integer primary key, 'node_rev_id' integer, 'field_id' integer, 'data' text, 'config' text, 'weight' integer );
create table 'files' ( 'id' integer primary key, 'user_id' integer, 'parent_file_id' integer, 'name' text, 'type' text, 'title' text, 'description' text, 'group' text, 'mode' text );
create table 'xxx' ( 'id' integer primary key, 'data' text);
create table 'views' ( 'id' integer primary key, 'name' text, 'format' text, 'header' text, 'footer' text, 'pager_items' text );
create table 'views_filter' ( 'id' integer primary key, 'view_id' integer, 'field_id' integer, 'operator' text, 'data' text, 'data_exposed' integer, 'filter_exposed' integer, 'operator_exposed' integer, 'required' integer );
create table 'views_sort' ( 'id' integer primary key, 'view_id' integer, 'field_id' integer, 'direction' text );
create table 'field_view' ( 'id' integer primary key, 'view_id' integer, 'field_id' integer, 'label' text, 'empty_hide' integer, 'config' text );

insert into views values( 1, 'display', 'list', '', '', '25' );
insert into views values( 2, 'teaser', 'list', '', '', '25' );
insert into views values( 3, 'rss', 'list', '', '', '25' );
insert into views values( 4, 'tracker', 'table', '', '', '25' );

insert into xxx values( 1, 'foo' );
insert into xxx values( 2, 'bar' );
insert into xxx values( 3, 'hello' );
insert into xxx values( 4, 'jhgjhgkjhgjkhf' );

insert into files values( 1, 2, 0, 'temp', 'd', 'Temp', 'Temporary files' );
insert into files values( 2, 2, 1, '002609.jpg', 'f', 'Mouse', 'Small mouse avatar' );

insert into node_type values( 1, 'page' );

insert into field_config values( 1, '1', 'tags', 'editbox', 'Enter a comma-separated list of words to describe your content.' );
insert into field_config values( 2, '1', 'body', 'textarea', 'The main body text of the node.' );

-- Special fields
insert into field_config values(1000, 0, "node_type_id", "editbox", "");
insert into field_config values(1001, 0, "node_type_name", "editbox", "");
insert into field_config values(1002, 0, "node_id", "editbox", "");
insert into field_config values(1003, 0, "node_user_id", "editbox", "");
insert into field_config values(1004, 0, "node_language", "language", "");
insert into field_config values(1005, 0, "node_published", "checkbox", "");
insert into field_config values(1006, 0, "node_promoted", "checkbox", "");
insert into field_config values(1007, 0, "node_sticky", "checkbox", "");
insert into field_config values(1008, 0, "node_rev_id", "editbox", "");
insert into field_config values(1009, 0, "node_rev_user_id", "editbox", "");
insert into field_config values(1010, 0, "node_rev_title", "editbox", "");
insert into field_config values(1011, 0, "node_rev_timestamp", "timestamp", "");
insert into field_config values(1012, 0, "node_rev_log_msg", "editbox", "");

insert into node values( 1, '2', '1', 'ru', '1', '1', '1', '0' );
insert into node_rev values( 1, '1', '2', 'мышь', '1344063303', '' );
insert into field_data values( null, '1', '1', 'писк');
insert into field_data values( null, '1', '2', 'фуу');

insert into node values( 2, '2', '1', 'en', '2', '1', '1', '0' );
insert into node_rev values( 2, '2', '2', 'mouse', '1344163303', '' );
insert into field_data values( null, '2', '1', 'squeak');
insert into field_data values( null, '2', '2', 'foo');

insert into node values( 3, '2', '1', 'de', '4', '0', '1', '0' );
insert into node_rev values( 3, '3', '2', 'die Maus', '1344263303', '' );
insert into field_data values( null, '3', '1', 'fiepseln');
insert into field_data values( null, '3', '2', 'föo');
insert into node_rev values( 4, '3', '2', 'der Mäuserich', '1344363303', 'ersetzte die männliche Maus' );
insert into field_data values( null, '4', '1', 'fieps');
insert into field_data values( null, '4', '2', 'föö');

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

insert into users values( 1, 'guest', 'Anonymous', '@guest', 'nologin' );
insert into users values( 2, 'admin', 'Administrator', '@admin', 'c7ad44cbad762a5da0a452f9e854fdc1e0e7a52a38015f23f3eab1d80b931dd472634dfac71cd34ebc35d16ab7fb8a90c81f975113d6c7538dc69dd8de9077ec' );
insert into users values( 3, 'test', 'Test', '@user', 'ee26b0dd4af7e749aa1a8ee3c10ae9923f618980772e473f8819a5d4940e0db27ac185f8a0e1d5f84f88bc887fd67b143732c304cc5fa9ad8e6f57f50028a8ff' );

insert into groups values( 1, 'admin', 'Administrator' );
insert into groups values( 2, 'user', 'Authenticated user' );
insert into groups values( 3, 'guest', 'Anonymous user' );
insert into groups values( 4, 'nobody', 'Nobody' );
insert into groups values( 5, 'owner', 'Access to own objects' );

insert into usergroups values( 1, 1, 3 );
insert into usergroups values( 2, 2, 1 );
insert into usergroups values( 3, 3, 2 );

insert into posts values( null, '1', '1232146914', '0', 'Test subject', 'Test text' );
insert into posts values( null, '1', '1261767564', '1', 'bla', 'blablabla' );
insert into posts values( null, '1', '1261767578', '1', 'xxx', '<i>italic text, <b>bold</b> and <u>underline</u></i>' );

insert into languages values( null, 'English', 'English', 'en', 0, 1 );
insert into languages values( null, 'Estonian', 'Eesti', 'et', 0, 1 );
insert into languages values( null, 'Russian', 'Русский', 'ru', 0, 1 );
insert into languages values( null, 'German', 'Deutsch', 'de', 0, 1 );
