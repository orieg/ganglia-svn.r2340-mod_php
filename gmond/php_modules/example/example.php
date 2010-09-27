<?php
/*******************************************************************************
*
* This code is part of a php module for ganglia.
*
* Author : Nicolas Brousse (nicolas brousse.info)
*
* Portions Copyright (C) 2007 Novell, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
*  - Neither the name of Novell, Inc. nor the names of its
*    contributors may be used to endorse or promote products derived from this
*    software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL Novell, Inc. OR THE CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

$descriptors = array();
$Random_Max = 50;
$Constant_Value = 50;

function Random_Numbers($name) {
    // Return a random number.
    global $Random_Max;
    echo "[php example] Random_Numbers\n";
    return rand(0, $Random_Max);
}

function Constant_Number($name) {
    // Return a constant number.
    global $Constant_Value;
    echo "[php example] Constant_Number\n";
    return (int) $Constant_Value;
}

function metric_init($params) {
    /* Initialize the random number generator and create the
    metric definition dictionary object for each metric. */
    global $descriptors;
    global $Random_Max;
    global $Constant_Value;
    
    echo "[php example] Received the following parameters:\n";
    var_dump($params);
    
    if (in_array('RandomMax', $params)) {
        $Random_Max = (int) $params['RandomMax'];
    }
    
    if (in_array('ConstantValue', $params)) {
        $Constant_Value = (int) $params['ConstantValue'];
    }
    
    $d1 = array(
        'name' => 'PHP_Random_Numbers',
        'call_back' => "Random_Numbers",
        'time_max' => 90,
        'value_type' => 'uint',
        'units' => 'N',
        'slope' => 'both',
        'format' => '%u',
        'description' => 'Example module metric (random numbers)',
        'groups' => 'example,random'
    );

    $d2 = array(
        'name' => 'PHP_Constant_Number',
        'call_back' => "Constant_Number",
        'time_max' => 90,
        'value_type' => 'uint',
        'units' => 'N',
        'slope' => 'zero',
        'format' => '%hu',
        'description' => 'Example module metric (constant number)'
    );

    $descriptors = array($d1, $d2);
    
    echo "[php example] Returning descriptors :\n";
    var_dump($descriptors);
    
    return $descriptors;
}

function metric_cleanup() {
    // Clean up the metric module.
    echo "[php example] metric cleanup\n";
}

#This code is for debugging and unit testing
if ($_SERVER['SAPI_TYPE'] != 'embed') {
	print "Non-embed mode\n";
    $params = array(
        'RandomMax' => '500',
        'ConstantValue' => '322'
    );
    metric_init($params);
    foreach ($descriptors as $d) {
    	$v = $d['call_back']($d['name']);
        printf("value for %s is %u\n", $d['name'], $v);
    }
}