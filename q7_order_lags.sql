select 'order'.id,orderdate as curdate,(lag(orderdate,1,orderdate) over(order by orderdate)) as predate,round(julianday(orderdate)-julianday(lag(orderdate,1,orderdate) over(order by orderdate)),2) as difference 
from 'order' 
where  customerid='BLONP' 
order by orderdate limit 10;
