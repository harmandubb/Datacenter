function hello(){
 command="$@"
 for word in "${command[@]}";
 do 
   echo $word
 done  
 echo "Hello the Bash is working"
}

export -f hello
