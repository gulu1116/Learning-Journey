DROP TABLE IF EXISTS `dl_account_t`;
CREATE TABLE `dl_account_t` (
	`id` INT(11) NOT NULL AUTO_INCREMENT,
	`name` VARCHAR(255) DEFAULT NULL,
	`money` INT(11) DEFAULT 0,
	`account_id` INT(11) DEFAULT 0,
	PRIMARY KEY (`id`),
	UNIQUE `uk_account_id` (`account_id`)
)ENGINE = INNODB AUTO_INCREMENT=0 DEFAULT CHARSET = utf8;

-- select * from dl_account_t
INSERT INTO `dl_account_t`(`name`,`money`,`account_id`) VALUES ('C', 1000, 10001),('B', 1000, 10002),('A', 1000, 10003),('ZZ', 1000, 10005)

-- 相反加锁顺序死锁1
SET TRANSACTION ISOLATION LEVEL REPEATABLE READ;
BEGIN
-- 死锁事务1
UPDATE `dl_account_t` SET `money` = `money` - 100 WHERE `id` = 1;
-- 死锁事务2
-- UPDATE `dl_account_t` SET `money` = `money` - 100 WHERE `id` = 2;
-- 死锁事务1
UPDATE `dl_account_t` SET `money` = `money` + 100 WHERE `id` = 2;
-- 死锁事务2
-- UPDATE `dl_account_t` SET `money` = `money` - 100 WHERE `id` = 1;

rollback

DROP TABLE IF EXISTS `dl_mark_t`;
CREATE TABLE `dl_mark_t` (
  `a` INT(11) NOT NULL DEFAULT '0',
  `b` INT(11) DEFAULT NULL,
  PRIMARY KEY (`a`),
  UNIQUE KEY `uk_b` (`b`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


INSERT INTO `dl_mark_t` VALUES (1,1),(5,4),(20,20),(25,12);
-- 死锁情况 1
/*
  事务一在插入时由于跟事务二插入的记录唯一键冲突，所以对 b=10 这个唯一索引加 S 锁（Next-key）并处于锁等待，事务二再插入 b=9 这条记录，需要获取插入意向锁（lock_mode X locks gap before rec insert intention）和事务一持有的 Next-key 锁冲突，从而导致死锁。
*/
BEGIN
-- 死锁事务 2
insert into `dl_mark_t` values(26,10);
-- 死锁事务 1
insert into `dl_mark_t` values(30,10);
-- 死锁事务 2
insert into `dl_mark_t` values(40,9);


-- 死锁情况 2
/*
1. 三个事务依次执行 insert 语句，由于 b是唯一索引，所以后两个事务会出现唯一键冲突。但此时要注意的是事务一还没有提交，所以并不会立即报错。事务二和事务三为了判断是否出现唯一键冲突，必须进行一次当前读，加的锁是 Next-Key 锁，所以进入锁等待。要注意的是，就算在 RC 隔离级别下，一样会加 Next-Key 锁，所以说出现 GAP 锁不一定就是 RR 隔离级别;
2. 事务一回滚，此时事务二和事务三成功获取记录上的 S 锁；
3. 事务二和事务三继续执行插入操作，需要依次请求记录上的插入意向锁（插入意向锁和 GAP 锁冲突，所以事务二等待事务三，事务三等待事务二，形成死锁。
*/
BEGIN
-- 死锁事务 1
insert into `dl_mark_t` values(27, 29);
-- 死锁事务 2
insert into `dl_mark_t` values(28, 29);
-- 死锁事务 3
insert into `dl_mark_t` values(29, 29);
-- 死锁事务 1
ROLLBACK;
