import pandas as pd
import sys

# Read chunks to handle large file
file_path = "output/spatial_data.csv"

# Columns: Time,AgentId,TownId,SchoolId,ReligiousId,WorkplaceId,ClaimId,State,IsMisinformation,Ethnicity,Denomination
# State: 0=S, 1=E, 2=D, 3=P, 4=N, 5=R

print("Analyzing demographics of infected agents...")

try:
    # Read only relevant columns to save memory
    usecols = ["Time", "ClaimId", "State", "Ethnicity", "Denomination"]
    
    # We only care about the final state or cumulative infections.
    # Let's look for any agent who is NOT Susceptible (State != 0)
    
    # Map for stats
    # Claim -> Ethnicity -> Count
    stats = {}
    
    # iterate in chunks
    chunksize = 100000
    for chunk in pd.read_csv(file_path, chunksize=chunksize, usecols=usecols):
        # Filter for infected/recovered (State > 0)
        infected = chunk[chunk["State"] > 0]
        
        if infected.empty:
            continue

        # Group by Claim, Ethnicity
        # We might count the same agent multiple times if we look at all times?
        # Yes. We should de-duplicate by AgentId but we didn't load it.
        # Let's just look at the distribution of "Infected Person-Days" or just the final snapshots.
        # Actually, let's load AgentId too.
        pass
    
    # Better approach: Just read the file, it's 50k agents * 365 steps = 18M rows. Might be big.
    # Let's just look at the last recorded time step.
    
    # Get last time
    # Check tail first? No, pandas is smart.
    # We will assume the file is ordered by Time.
    # We'll read the whole thing but filter rows immediately? No.
    
    # Strategy: Read with iterator, keep track of max time.
    # Or just use the 'simulation_results.csv' for high level, but we want demographics.
    
    # Let's try reading just the infected rows.
    # pd.read_csv(..., iterator=True)
    
    counts = {} # (Claim, Eth, Denom) -> Set(AgentId)
    
    for chunk in pd.read_csv(file_path, chunksize=chunksize):
        # Filter
        mask = chunk["State"] > 0 # Not Susceptible
        relevant = chunk[mask]
        
        for _, row in relevant.iterrows():
            cid = row["ClaimId"]
            aid = row["AgentId"]
            eth = row["Ethnicity"]
            den = row["Denomination"]
            
            key = (cid, eth, den)
            if key not in counts:
                counts[key] = set()
            counts[key].add(aid)

    print("\n--- Demographics of Infected/Recovered Agents ---")
    
    # 0=White, 1=Hisp, 2=Black, 3=Asian, 4=Native, 5=Multi
    eth_names = {0: "White", 1: "Hispanic", 2: "Black", 3: "Asian", 4: "Native", 5: "Multi"}
    
    for key, agents in counts.items():
        cid, eth, den = key
        count = len(agents)
        if count > 10: # Only print significant clusters
            print(f"Claim {cid}, Eth {eth_names.get(eth, eth)}, Denom {den}: {count} unique agents")

except Exception as e:
    print(f"Error: {e}")
