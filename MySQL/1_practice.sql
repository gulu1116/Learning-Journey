SELECT A.student_id from

(SELECT student_id, num FROM score where course_id = 
  (SELECT cid FROM course WHERE cname = 'c++')) AS A
LEFT JOIN
(SELECT student_id, num FROM score where course_id = 
  (SELECT cid FROM course WHERE cname = '数据库')) AS B
ON A.student_id = B.student_id

where A.num > B.num;