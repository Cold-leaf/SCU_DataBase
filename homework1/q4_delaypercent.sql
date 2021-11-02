select  companyname,round(100.0*sum(case when  ShippedDate > RequiredDate then 1 else 0 end)/count(*),2) as lateperc from 'order',shipper where shipper.id=shipvia and shipvia=1
union
select  companyname,round(100.0*sum(case when  ShippedDate > RequiredDate then 1 else 0 end)/count(*),2) as lateperc from 'order',shipper where shipper.id=shipvia and shipvia=2
union
select  companyname,round(100.0*sum(case when  ShippedDate > RequiredDate then 1 else 0 end)/count(*),2) as lateperc from 'order',shipper where shipper.id=shipvia and shipvia=3
order by lateperc desc;
