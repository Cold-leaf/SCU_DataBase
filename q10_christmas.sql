select group_concat(productname) as namelist 
from(select distinct product.id,productname 
     from product,(select productid, 'order'.id,orderdate 
                   from orderdetail,customer,'order' 
                   where orderdate like '2014-12-25%' and 'order'.id=orderid and customer.id=customerid  and companyname='Queen Cozinha') as O 
     where productid=product.id order by product.id);
