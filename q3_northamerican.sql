select id, shipcountry ,replace(shipregion,shipregion,'Other Place') as region from 'order' where shipregion<>'North America'
union
select id, shipcountry ,shipregion as region from 'order' where shipregion='North America'
order by id limit 20 offset 5197;
