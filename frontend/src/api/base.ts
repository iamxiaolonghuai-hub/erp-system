import { get, post, put, del } from '@/utils/request'
import type { Supplier, Customer, Employee, Warehouse, Category } from '@/types'

// ============ 供应商 ============
export function getSuppliers(params?: object) {
  return get<Supplier[]>('/suppliers', params)
}
export function createSupplier(data: Partial<Supplier>) {
  return post<Supplier>('/suppliers', data)
}
export function updateSupplier(id: number, data: Partial<Supplier>) {
  return put(`/suppliers/${id}`, data)
}
export function deleteSupplier(id: number) {
  return del(`/suppliers/${id}`)
}

// ============ 客户 ============
export function getCustomers(params?: object) {
  return get<Customer[]>('/customers', params)
}
export function createCustomer(data: Partial<Customer>) {
  return post<Customer>('/customers', data)
}
export function updateCustomer(id: number, data: Partial<Customer>) {
  return put(`/customers/${id}`, data)
}
export function deleteCustomer(id: number) {
  return del(`/customers/${id}`)
}

// ============ 员工 ============
export function getEmployees(params?: object) {
  return get<Employee[]>('/employees', params)
}
export function createEmployee(data: Partial<Employee>) {
  return post<Employee>('/employees', data)
}
export function updateEmployee(id: number, data: Partial<Employee>) {
  return put(`/employees/${id}`, data)
}
export function deleteEmployee(id: number) {
  return del(`/employees/${id}`)
}

// ============ 仓库 ============
export function getWarehouses(params?: object) {
  return get<Warehouse[]>('/warehouses', params)
}
export function createWarehouse(data: Partial<Warehouse>) {
  return post<Warehouse>('/warehouses', data)
}
export function updateWarehouse(id: number, data: Partial<Warehouse>) {
  return put(`/warehouses/${id}`, data)
}
export function deleteWarehouse(id: number) {
  return del(`/warehouses/${id}`)
}

// ============ 商品分类 ============
export function getCategories(params?: object) {
  return get<Category[]>('/categories', params)
}
export function createCategory(data: Partial<Category>) {
  return post<Category>('/categories', data)
}
export function updateCategory(id: number, data: Partial<Category>) {
  return put(`/categories/${id}`, data)
}
export function deleteCategory(id: number) {
  return del(`/categories/${id}`)
}
