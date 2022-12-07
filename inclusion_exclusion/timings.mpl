# generates benchmarks timings (10 times unless timeout) for each of the listed files
# saves output to results.txt
# requires sat.mpl to be in the same directory (as in https://sites.math.rutgers.edu/~az202/Z/sat/sat.txt)
# these lines (to be run in a shell) run maple with a logfile that is updating while running
# and then, after exiting maple with the quit command, send a ping and the contents of the results file to a slack webhook url. 
# ```
# maple --echofile=testing.log; 
# curl -X POST -H 'Content-type: application/json' --data '{"text":"Finished running maple: <@SLACK_ID>"}' WEBHOOK_URL; 
# curl -X POST -H 'Content-type: application/json' --data '{"text":"'"$(cat results.txt | tr -d "\n" | sed 's/"/\\\"/g' | sed -E 's/\\([0-9])/\1/g' | sed -E 's/\\\././g')"'"}' WEBHOOK_URL;
# ```
# expects the file structure to be "current_dir/ShaTar Testing/test_name test cases/test_file.cnf"

read "sat.mpl";

files := [["factoring test cases", ["5.cnf", "7.cnf"]], ["pigeonhole test cases", ["p-1.cnf", "p-2.cnf", "p-3.cnf"]], ["random test cases", ["100v150c.cnf", "100v200c.cnf", "200v300c.cnf", "200v400c.cnf", "300v450c.cnf", "300v600c.cnf", "500v1000c.cnf", "500v750c.cnf"]], ["toughsat test cases", ["tsat30v30c.cnf", "tsat30v35c.cnf", "tsat30v40c.cnf", "tsat30v45c.cnf", "tsat30v50c.cnf", "tsat80v30c.cnf", "tsat90v30c.cnf"]]];

output := [];

for test_group in files do
	group_name := test_group[1];
	for file in test_group[2] do
		print(file);
		data := readdata("ShaTar Testing/" || group_name || "/" || file, integer, 99999);
		set_data := {};
		for i in data do
			set_data := set_data union {convert(i[1..(nops(i) - 1)], set)};
		end do;
		set_data := set_data minus {{}};

		test_output := [];
		for i in $(1..10) do
			t := time();

			try
				timelimit(60 * 10, Taut(set_data, 9999));
			catch:
				# timed out
				test_output := [op(test_output), "Timed out"];
				print("Timed out");
				break;
			end try;

			duration := time() - t;
			test_output := [op(test_output), duration];

			print("Took " || duration || " for " || file);
		end do;			
	
		output := [op(output), ["" || group_name || " " || file, test_output]];

		save output, "results.txt";
	end do;
end do;

quit;
;
