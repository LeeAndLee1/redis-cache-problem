# redis-cache-problem
The project reproduces cache penetration, cache breakdown, cache avalanche and data consistency problems and provides solutions by c++.

1. the first step, you need to create a database named 'redisTestDB' by mysql.
  create database redisTestDB  charset=utf8;
2. the second step, you need to create a table named 'student' in 'redisTestDB', for example:
  create table `student` ( 
    `id` int(11) not null, 
    `name` varchar(50) default null, 
    `age` int(11) not null, 
    primary key (`id`), 
    unique key `name` (`name`) 
  )default charset=utf8;
3. the third step, you need to insert some lines into 'student' to repeat the question above, for example:
  insert into student(id, name, age) values(1, 'liming', 19);
  insert into student(id, name, age) values(2, 'xiaoming', 18);
4. the fourth step, you need to run the 'cmake' command to generate an executable files in 'bin' folder:
  mkdir build
  cd build && cmake ..
  make
5. then you can exacute the generated files to test.
