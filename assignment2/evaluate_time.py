import subprocess
import statistics

# Number of time to execute the program, for more precise average time
num_runs = 1
if num_runs==1:
    print(f"Computing wall time based on {num_runs} execution...")
else:
    print(f"Computing average wall time based on {num_runs} executions...")

# Maximum number of usable processors in your computer
max_processors = 4
serial_time = 0
for i in range(max_processors):
    # Command to execute the MPI program
    mpi_command = "./scripts/launch.sh ./build/main ./data/molecules_hiv.smi output.csv " + str(i+1)
    real_times = []
    
    for _ in range(num_runs):
        result = subprocess.run(mpi_command, shell=True, capture_output=True, text=True)        
        output_lines = result.stderr.strip().split("\n")

        # Exctract from stderr the wall time and convert it
        real_time_str = output_lines[-3].split()[1]
        
        minutes_str, seconds_str = real_time_str.split('m')
        minutes = int(minutes_str)
        seconds = float(seconds_str.rstrip('s'))

        total_seconds = minutes * 60 + seconds

        real_times.append(total_seconds)

    average_real_time = statistics.mean(real_times)
    
    minutes_result = int(average_real_time / 60)
    seconds_result = round(average_real_time % 60, 3)
    if i==0:
        serial_time = average_real_time

    scale = round(serial_time / average_real_time, 3)
    print(f"n_proc={i+1}, time: {minutes_result} min, {seconds_result} s, scale: {scale}")
