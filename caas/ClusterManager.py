from PizzaService import PizzaService
from ServiceRegistry import ServiceRegistry
from IdempotencyStore import IdempotencyStore
import Constants

# One store shared by every replica (stands in for Redis / a DB)
idempotency_store = IdempotencyStore()

# 1. Running pizza service replicas on machines 1 and 2
# TODO: Dynamically assign a machine
replica_1 = PizzaService("Replica 1 @ 127.0.0.1:8080", Constants.machines_info[1][0], Constants.machines_info[1][1], idempotency_store)
replica_2 = PizzaService("Replica 2 @ 127.0.0.1:7070", Constants.machines_info[2][0], Constants.machines_info[2][1], idempotency_store)
replica_3 = PizzaService("Replica 3 @ 127.0.0.1:6060", Constants.machines_info[3][0], Constants.machines_info[3][1], idempotency_store)

registry = ServiceRegistry()
registry.register("pizza-service", Constants.machines_info[1][0], Constants.machines_info[1][1])
registry.register("pizza-service", Constants.machines_info[2][0], Constants.machines_info[2][1])
registry.register("pizza-service", Constants.machines_info[3][0], Constants.machines_info[3][1])

# ClusterManager -> N Machines
# -> ServiceRegistry

