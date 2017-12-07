<?php

	function endsWith($haystack, $needle){
		$length = strlen($needle);
		$start  = $length * -1; //negative
		return (substr($haystack, $start) === $needle);
	}
	
	function getFilesFromDir($dir, array & $files) { 
		if ($handle = opendir($dir)) { 
			while (false !== ($file = readdir($handle))) { 
				if ($file != "." && $file != "..") { 
					if(is_dir($dir.'/'.$file)) { 
						$dir2 = $dir.'/'.$file; 
						getFilesFromDir($dir2, $files); 
					} 
					else { 
						if (endsWith(strtoupper($file), '.H')){
							$files[] = $dir.'/'.$file;
						}
					} 
				} 
			} 
			closedir($handle); 
		} 
		
		return $files; 
	}
	
	function getMethodsFromFile(&$runner, $file){
		if (($contents = file_get_contents($file)) !== false){
			//get the class name
			if (($pos = strpos($contents, 'class ')) !== false){
				if (($pos2 = strpos($contents, ':', $pos)) !== false){
					$class = trim(substr($contents, $pos + 6, $pos2 - $pos - 6));
					if (strpos($class, ' ') === false){
						$runner .= '    {' . PHP_EOL . '        ' . $class . ' ttt("' . $class . '");' . PHP_EOL;
						
						//get the methods
						while(($pos = strpos($contents, 'void test', $pos)) !== false){
							$pos2 = strpos($contents, '(', $pos);
							$method = substr($contents, $pos + 5, $pos2 - $pos - 5);
							$runner .= '        RUNTEST(' . $method . ');' . PHP_EOL;
							$pos = $pos2;
						}
						$runner .= '    }' . PHP_EOL;
					}
				}
			}
		} else {
			$runner .= '    //' . $file . ' cound not be opened' . PHP_EOL;
		}
	}

	$array = array();
	getFilesFromDir(dirname(dirname(__FILE__)) . '/TestSuite2', $array);
	$runner = '#include "RunTestSuite2.h"' . PHP_EOL;
	foreach($array as $file){
		$runner .= '#include "' . $file . '"' . PHP_EOL;
	}
	$runner .= PHP_EOL . '#define RUNTEST(name) ttt.setUp(#name); ttt.name(); ttt.tearDown()' . PHP_EOL;
	$runner .= PHP_EOL . 'void RunTestSuite2::RunTests(void){' . PHP_EOL;

	foreach($array as $file){
		getMethodsFromFile($runner, $file);
	}
	

	$runner .= '}' . PHP_EOL . PHP_EOL;
	if (file_put_contents(dirname(__FILE__) . '/RunTestSuite2.cpp', $runner) === false){
		echo 'Could not write to output file' . PHP_EOL;
	} else {
		echo 'Done' . PHP_EOL;
	}