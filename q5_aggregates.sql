select * 
from (select categoryname,count(*) as count,round(avg(unitprice),2) as avprice,min(unitprice) as minprice,max(unitprice) as maxprice,sum(unitsonorder) as sumorder 
      from category,product 
      where categoryid=category.id 
      group by categoryid) 
where count>10;
