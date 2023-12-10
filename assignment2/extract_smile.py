import pandas as pd

filenamecsv = 'HIV.csv'
csv = pd.read_csv(filenamecsv, sep=',')
smiles = csv['smiles']
filename = './data/molecules_hiv.smi'
smiles.to_csv(filename, index=False, header=False)