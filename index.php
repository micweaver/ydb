<?php


class CB  {
	private $pv = 'abc';
	function add($a,$b){
		 return $a+$b;
	}
}

class CA {

	public function look_value($pa,$pb){
	   
		$fa = $pa;
        $fb = $pb;
        $fc = $fa+$fb;
       
    	$arr = array(1,2,3);
        $objCb = new CB();
        $fd = $objCb->add(3,4);
 
    	$arr = array(4,5,6);
		return $fd;
	}
	
	function look_cost(){
		$this->fun1();
		$this->fun2();
		$this->fun3();
		usleep(300000);
		
	}
	
	function fun1(){
		usleep(100000);
		return;
	}
	function fun2(){
		usleep(200000);
		return;
	}
	
	function fun3(){
		usleep(500000);
		return;
	}
}


$objAt = new CA();
$res = $objAt->look_value(1, 2);

$res = $objAt->look_cost();

echo "should not be here";


