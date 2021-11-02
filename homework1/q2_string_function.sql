select distinct replace(shipname,shipname,shipname||'|'||substring(shipname,1,instr(shipname,'-')-1)) as name 
from 'order' 
where shipname like '%-%'order by shipname;
