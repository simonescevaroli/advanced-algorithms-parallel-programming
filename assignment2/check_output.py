import pandas as pd

real_data = pd.read_csv("output_real.csv", sep=" ")
my_data = pd.read_csv("output_mine.csv", sep=" ")

real_data.columns = real_data.columns.str.strip()
my_data.columns = my_data.columns.str.strip()

# Sort the dataframe based on the column "NGRAM"
real_data = real_data.sort_values(by="NGRAM").reset_index(drop=True)
my_data = my_data.sort_values(by="NGRAM").reset_index(drop=True)

# Check if match
differences = real_data.compare(my_data)
if differences.empty:
    print("The files are identical.")
else:
    print("The differences are:")
    print(differences)
