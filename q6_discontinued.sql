select productname,companyname,contactname 
from(select productname,companyname,contactname,min(orderdate) 
     from orderdetail,(select 'order'.id,companyname,contactname,orderdate from customer,'order' 
                        where customer.id=customerid) as O,product 
     where orderid=O.id and product.id=productid and discontinued=1 
     group by productname 
     order by productname) ;
