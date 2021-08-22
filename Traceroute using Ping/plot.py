import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("output.csv")
df1 = df.drop(columns=["IP"])
df1["RTT"] = df1["RTT"].apply(lambda x:x/1000)
plt.plot(df1["Hop"],df1["RTT"],color="red")
plt.xlabel("Hop Number(TTL value)", color="black")
plt.ylabel("Round Trip Time", color="black")
plt.title("RTT v/s Hop Number", color="blue")
plt.show()