select companyname,id,expenditures 
from (select companyname,customer.id,round(sum(unitprice*quantity),2) as expenditures,ntile(4) over(order by sum(unitprice*quantity)) as nt 
      from customer,'order',orderdetail 
      where customer.id=customerid and 'order'.id=orderid 
      group by companyname) 
where nt=1;
