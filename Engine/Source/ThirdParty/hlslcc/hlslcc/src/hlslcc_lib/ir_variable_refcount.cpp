// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

// This code is modified from that in the Mesa3D Graphics library available at
// http://mesa3d.org/
// The license for the original code follows:

/*
* Copyright © 2010 Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

/**
* \file ir_variable_refcount.cpp
*
* Provides a visitor which produces a list of variables referenced,
* how many times they were referenced and assigned, and whether they
* were defined in the scope.
*/

#include "ShaderCompilerCommon.h"
#include "ir.h"
#include "ir_visitor.h"
#include "ir_variable_refcount.h"
#include "glsl_types.h"


// constructor
ir_variable_refcount_entry::ir_variable_refcount_entry(ir_variable *var)
{
	this->var = var;
	assign = NULL;
	assigned_count = 0;
	declaration = false;
	referenced_count = 0;
}


ir_variable_refcount_entry *
ir_variable_refcount_visitor::get_variable_entry(ir_variable *var)
{
	check(var);
	if (Variables.find(var) == Variables.end())
	{
		ir_variable_refcount_entry *entry = new(mem_ctx)ir_variable_refcount_entry(var);
		check(entry->referenced_count == 0);
		Variables[var] = entry;

	}
	return Variables[var];
}


ir_visitor_status
ir_variable_refcount_visitor::visit(ir_variable *ir)
{
	ir_variable_refcount_entry *entry = this->get_variable_entry(ir);
	if (entry)
	{
		entry->declaration = true;
	}

	return visit_continue;
}


ir_visitor_status
ir_variable_refcount_visitor::visit(ir_dereference_variable *ir)
{
	ir_variable *const var = ir->variable_referenced();
	ir_variable_refcount_entry *entry = this->get_variable_entry(var);

	if (entry)
	{
		entry->referenced_count++;
		entry->last_assign = NULL;
	}

	return visit_continue;
}


ir_visitor_status
ir_variable_refcount_visitor::visit_enter(ir_function_signature *ir)
{
	/* We don't want to descend into the function parameters and
	* dead-code eliminate them, so just accept the body here.
	*/
	visit_list_elements(this, &ir->body);
	return visit_continue_with_parent;
}


ir_visitor_status
ir_variable_refcount_visitor::visit_leave(ir_assignment *ir)
{
	ir_variable_refcount_entry *entry;
	entry = this->get_variable_entry(ir->lhs->variable_referenced());
	if (entry)
	{
		entry->assigned_count++;
		if (entry->assign == NULL)
		{
			entry->assign = ir;
		}
		if (control_flow_depth == 0)
		{
			entry->last_assign = ir;
		}
		else
		{
			entry->last_assign = NULL;
		}
	}

	return visit_continue;
}

ir_visitor_status
ir_variable_refcount_visitor::visit_leave(ir_call *ir)
{
	if (ir->return_deref)
	{
		ir_variable_refcount_entry *entry;
		entry = this->get_variable_entry(ir->return_deref->variable_referenced());
		if (entry)
		{
			entry->assigned_count++;
			if (entry->call == NULL)
			{
				entry->call = ir;
			}
		}
	}

	return visit_continue;
}

ir_visitor_status
ir_variable_refcount_visitor::visit_enter(ir_if *ir)
{
	control_flow_depth++;
	return visit_continue;
}

ir_visitor_status
ir_variable_refcount_visitor::visit_leave(ir_if *ir)
{
	control_flow_depth--;
	return visit_continue;
}

ir_visitor_status
ir_variable_refcount_visitor::visit_enter(ir_loop *ir)
{
	control_flow_depth++;
	return visit_continue;
}

ir_visitor_status
ir_variable_refcount_visitor::visit_leave(ir_loop *ir)
{
	control_flow_depth--;
	return visit_continue;
}
