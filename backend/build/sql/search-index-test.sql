
INSERT INTO indexed_resources (
	url, title, type
) VALUES
	('/p?id=12', 'Трент Резнор', 'person');


INSERT INTO search_index (resource_id, keyword, value)
SELECT id, 'nine', 1
FROM indexed_resources WHERE url='/p?id=12';

INSERT INTO search_index (resource_id, keyword, value)
SELECT id, 'inch', 1
FROM indexed_resources WHERE url='/p?id=12';

INSERT INTO search_index (resource_id, keyword, value)
SELECT id, 'nails', 1
FROM indexed_resources WHERE url='/p?id=12';

INSERT INTO search_index (resource_id, keyword, value)
SELECT id, 'трент', 1
FROM indexed_resources WHERE url='/p?id=12';

INSERT INTO search_index (resource_id, keyword, value)
SELECT id, 'резнор', 1
FROM indexed_resources WHERE url='/p?id=12';

INSERT INTO search_index (resource_id, keyword, value)
SELECT id, 'trent', 1
FROM indexed_resources WHERE url='/p?id=12';

INSERT INTO search_index (resource_id, keyword, value)
SELECT id, 'reznor', 1
FROM indexed_resources WHERE url='/p?id=12';
