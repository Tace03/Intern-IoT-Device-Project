PRAGMA foreign_keys=OFF;

DROP TABLE IF EXISTS errorLog;
CREATE TABLE errorLog (
timestamp  INTEGER,
event  TEXT NOT NULL
);

