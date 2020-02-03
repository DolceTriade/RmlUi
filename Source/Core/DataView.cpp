/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "precompiled.h"
#include "../../Include/RmlUi/Core/DataModel.h"
#include "../../Include/RmlUi/Core/DataView.h"
#include "DataParser.h"

namespace Rml {
namespace Core {

DataView::~DataView() {}

Element* DataView::GetElement() const
{
	Element* result = attached_element.get();
	if (!result)
		Log::Message(Log::LT_WARNING, "Could not retrieve element in view, was it destroyed?");
	return result;
}

int DataView::GetElementDepth() const {
	return element_depth;
}

bool DataView::IsValid() const {
	return (bool)attached_element;
}

DataView::DataView(Element* element) : attached_element(element->GetObserverPtr()), element_depth(0) {
	if (element)
	{
		for (Element* parent = element->GetParentNode(); parent; parent = parent->GetParentNode())
			element_depth += 1;
	}
}

void DataView::Release()
{
	delete this;
}




DataViewNamedExpression::DataViewNamedExpression(Element* element) : DataView(element)
{}

DataViewNamedExpression::~DataViewNamedExpression()
{}

bool DataViewNamedExpression::Initialize(DataModel& model, Element* element, const String& data_expression_str, const String& in_view_label)
{
	view_label = in_view_label;
	data_expression = std::make_unique<DataExpression>(data_expression_str);
	DataExpressionInterface interface(&model, element);

	bool result = data_expression->Parse(interface);
	return result;
}

StringList DataViewNamedExpression::GetVariableNameList() const {
	RMLUI_ASSERT(data_expression);
	return data_expression->GetVariableNameList();
}

const String& DataViewNamedExpression::GetViewLabel() const {
	return view_label;
}

DataExpression& DataViewNamedExpression::GetDataExpression() {
	RMLUI_ASSERT(data_expression);
	return *data_expression;
}



DataViewAttribute::DataViewAttribute(Element* element) : DataViewNamedExpression(element)
{}

DataViewAttribute::~DataViewAttribute()
{}

bool DataViewAttribute::Update(DataModel& model)
{
	const String& attribute_name = GetViewLabel();
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface interface(&model, element);

	if (element && GetDataExpression().Run(interface, variant))
	{
		const String value = variant.Get<String>();
		const Variant* attribute = element->GetAttribute(attribute_name);
		
		if (!attribute || (attribute && attribute->Get<String>() != value))
		{
			element->SetAttribute(attribute_name, value);
			result = true;
		}
	}
	return result;
}


DataViewStyle::DataViewStyle(Element* element) : DataViewNamedExpression(element)
{}

DataViewStyle::~DataViewStyle()
{
}


bool DataViewStyle::Update(DataModel& model)
{
	const String& property_name = GetViewLabel();
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface interface(&model, element);
	
	if (element && GetDataExpression().Run(interface, variant))
	{
		const String value = variant.Get<String>();
		const Property* p = element->GetLocalProperty(property_name);
		if (!p || p->Get<String>() != value)
		{
			element->SetProperty(property_name, value);
			result = true;
		}
	}
	return result;
}


DataViewClass::DataViewClass(Element* element) : DataViewNamedExpression(element)
{}

DataViewClass::~DataViewClass()
{}

bool DataViewClass::Update(DataModel& model)
{
	const String& class_name = GetViewLabel();
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface interface(&model, element);

	if (element && GetDataExpression().Run(interface, variant))
	{
		const bool activate = variant.Get<bool>();
		const bool is_set = element->IsClassSet(class_name);
		if (activate != is_set)
		{
			element->SetClass(class_name, activate);
			result = true;
		}
	}
	return result;
}


DataViewRml::DataViewRml(Element* element) : DataViewNamedExpression(element)
{}

DataViewRml::~DataViewRml()
{}

bool DataViewRml::Update(DataModel & model)
{
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface interface(&model, element);

	if (element && GetDataExpression().Run(interface, variant))
	{
		String new_rml = variant.Get<String>();
		if (new_rml != previous_rml)
		{
			element->SetInnerRML(new_rml);
			previous_rml = std::move(new_rml);
			result = true;
		}
	}
	return result;
}


DataViewIf::DataViewIf(Element* element) : DataViewNamedExpression(element)
{}

DataViewIf::~DataViewIf()
{}


bool DataViewIf::Update(DataModel& model)
{
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface interface(&model, element);

	if (element && GetDataExpression().Run(interface, variant))
	{
		const bool value = variant.Get<bool>();
		const bool is_visible = (element->GetLocalStyleProperties().count(PropertyId::Display) == 0);
		if(is_visible != value)
		{
			if (value)
				element->RemoveProperty(PropertyId::Display);
			else
				element->SetProperty(PropertyId::Display, Property(Style::Display::None));
			result = true;
		}
	}
	return result;
}







DataViewText::DataViewText(Element* element) : DataView(element)
{

}

DataViewText::~DataViewText()
{}

bool DataViewText::Initialize(DataModel& model, Element* element, const String& RMLUI_UNUSED_PARAMETER(data_expression), const String& RMLUI_UNUSED_PARAMETER(view_label))
{
	RMLUI_UNUSED(data_expression);
	RMLUI_UNUSED(view_label);

	ElementText* element_text = rmlui_dynamic_cast<ElementText*>(element);
	if (!element_text)
		return false;

	const String& in_text = element_text->GetText();
	
	text.reserve(in_text.size());

	DataExpressionInterface expression_interface(&model, element);

	size_t previous_close_brackets = 0;
	size_t begin_brackets = 0;
	while ((begin_brackets = in_text.find("{{", begin_brackets)) != String::npos)
	{
		text.insert(text.end(), in_text.begin() + previous_close_brackets, in_text.begin() + begin_brackets);

		const size_t begin_name = begin_brackets + 2;
		const size_t end_name = in_text.find("}}", begin_name);

		if (end_name == String::npos)
			return false;

		DataEntry entry;
		entry.index = text.size();
		entry.data_expression = std::make_unique<DataExpression>(String(in_text.begin() + begin_name, in_text.begin() + end_name));

		if (entry.data_expression->Parse(expression_interface))
			data_entries.push_back(std::move(entry));

		previous_close_brackets = end_name + 2;
		begin_brackets = previous_close_brackets;
	}

	if (data_entries.empty())
		return false;

	if (previous_close_brackets < in_text.size())
		text.insert(text.end(), in_text.begin() + previous_close_brackets, in_text.end());

	return true;
}

bool DataViewText::Update(DataModel& model)
{
	bool entries_modified = false;
	Element* element = GetElement();
	DataExpressionInterface expression_interface(&model, element);

	for (DataEntry& entry : data_entries)
	{
		RMLUI_ASSERT(entry.data_expression);
		Variant variant;
		bool result = entry.data_expression->Run(expression_interface, variant);
		const String value = variant.Get<String>();
		if (result && entry.value != value)
		{
			entry.value = value;
			entries_modified = true;
		}
	}

	if (entries_modified)
	{
		if (Element* element = GetElement())
		{
			RMLUI_ASSERTMSG(rmlui_dynamic_cast<ElementText*>(element), "Somehow the element type was changed from ElementText since construction of the view. Should not be possible?");

			if (auto text_element = static_cast<ElementText*>(element))
			{
				String new_text = BuildText();
				text_element->SetText(new_text);
			}
		}
		else
		{
			Log::Message(Log::LT_WARNING, "Could not update data view text, element no longer valid. Was it destroyed?");
		}
	}

	return entries_modified;
}

StringList DataViewText::GetVariableNameList() const
{
	StringList full_list;
	full_list.reserve(data_entries.size());

	for (const DataEntry& entry : data_entries)
	{
		RMLUI_ASSERT(entry.data_expression);

		StringList entry_list = entry.data_expression->GetVariableNameList();
		full_list.insert(full_list.end(),
			std::make_move_iterator(entry_list.begin()),
			std::make_move_iterator(entry_list.end())
		);
	}

	return full_list;
}

String DataViewText::BuildText() const
{
	size_t reserve_size = text.size();

	for (const DataEntry& entry : data_entries)
		reserve_size += entry.value.size();

	String result;
	result.reserve(reserve_size);

	size_t previous_index = 0;
	for (const DataEntry& entry : data_entries)
	{
		result += text.substr(previous_index, entry.index - previous_index);
		result += entry.value;
		previous_index = entry.index;
	}

	if (previous_index < text.size())
		result += text.substr(previous_index);

	return result;
}






DataViewFor::DataViewFor(Element* element) : DataView(element)
{}

DataViewFor::~DataViewFor()
{}

bool DataViewFor::Initialize(DataModel& model, Element* element, const String& in_binding_name, const String& in_rml_content)
{
	rml_contents = in_rml_content;

	StringList binding_list;
	StringUtilities::ExpandString(binding_list, in_binding_name, ':');

	if (binding_list.empty() || binding_list.size() > 2 || binding_list.front().empty() || binding_list.back().empty())
	{
		Log::Message(Log::LT_WARNING, "Invalid syntax in data-for '%s'", in_binding_name.c_str());
		return false;
	}

	if (binding_list.size() == 2)
		alias_name = binding_list.front();
	else
		alias_name = "it";

	const String& binding_name = binding_list.back();

	variable_address = model.ResolveAddress(binding_name, element);
	if (variable_address.empty())
		return false;

	attributes = element->GetAttributes();
	attributes.erase("data-for");
	element->SetProperty(PropertyId::Display, Property(Style::Display::None));

	return true;
}



bool DataViewFor::Update(DataModel& model)
{
	Variable variable = model.GetVariable(variable_address);
	if (!variable)
		return false;

	bool result = false;
	const int size = variable.Size();
	const int num_elements = (int)elements.size();
	Element* element = GetElement();

	for (int i = 0; i < Math::Max(size, num_elements); i++)
	{
		if (i >= num_elements)
		{
			ElementPtr new_element_ptr = Factory::InstanceElement(nullptr, element->GetTagName(), element->GetTagName(), attributes);

			DataAddress replacement_address;
			replacement_address.reserve(variable_address.size() + 1);
			replacement_address = variable_address;
			replacement_address.push_back(AddressEntry(i));

			model.InsertAlias(new_element_ptr.get(), alias_name, replacement_address);

			Element* new_element = element->GetParentNode()->InsertBefore(std::move(new_element_ptr), element);
			elements.push_back(new_element);

			elements[i]->SetInnerRML(rml_contents);

			RMLUI_ASSERT(i < (int)elements.size());
		}
		if (i >= size)
		{
			model.EraseAliases(elements[i]);
			elements[i]->GetParentNode()->RemoveChild(elements[i]).reset();
			elements[i] = nullptr;
		}
	}

	if (num_elements > size)
		elements.resize(size);

	return result;
}

StringList DataViewFor::GetVariableNameList() const {
	return variable_address.empty() ? StringList() : StringList{ variable_address.front().name };
}

DataViews::DataViews()
{}

DataViews::~DataViews()
{}

void DataViews::Add(DataViewPtr view) {
	views_to_add.push_back(std::move(view));
}

void DataViews::OnElementRemove(Element* element) 
{
	for (auto it = views.begin(); it != views.end();)
	{
		auto& view = *it;
		if (view && view->GetElement() == element)
		{
			views_to_remove.push_back(std::move(view));
			it = views.erase(it);
		}
		else
			++it;
	}
}

bool DataViews::Update(DataModel & model, const SmallUnorderedSet< String >& dirty_variables)
{
	bool result = false;

	// View updates may result in newly added views, thus we do it recursively but with an upper limit.
	//   Without the loop, newly added views won't be updated until the next Update() call.
	for(int i = 0; i == 0 || (!views_to_add.empty() && i < 10); i++)
	{
		std::vector<DataView*> dirty_views;

		if (!views_to_add.empty())
		{
			views.reserve(views.size() + views_to_add.size());
			for (auto&& view : views_to_add)
			{
				dirty_views.push_back(view.get());
				for (const String& variable_name : view->GetVariableNameList())
					name_view_map.emplace(variable_name, view.get());

				views.push_back(std::move(view));
			}
			views_to_add.clear();
		}

		for (const String& variable_name : dirty_variables)
		{
			auto pair = name_view_map.equal_range(variable_name);
			for (auto it = pair.first; it != pair.second; ++it)
				dirty_views.push_back(it->second);
		}

		// Remove duplicate entries
		std::sort(dirty_views.begin(), dirty_views.end());
		auto it_remove = std::unique(dirty_views.begin(), dirty_views.end());
		dirty_views.erase(it_remove, dirty_views.end());

		// Sort by the element's depth in the document tree so that any structural changes due to a changed variable are reflected in the element's children.
		// Eg. the 'data-for' view will remove children if any of its data variable array size is reduced.
		std::sort(dirty_views.begin(), dirty_views.end(), [](auto&& left, auto&& right) { return left->GetElementDepth() < right->GetElementDepth(); });

		for (DataView* view : dirty_views)
		{
			RMLUI_ASSERT(view);
			if (!view)
				continue;

			if (view->IsValid())
				result |= view->Update(model);
		}

		// Destroy views marked for destruction
		// @performance: Horrible...
		if (!views_to_remove.empty())
		{
			for (const auto& view : views_to_remove)
			{
				for (auto it = name_view_map.begin(); it != name_view_map.end(); )
				{
					if (it->second == view.get())
						it = name_view_map.erase(it);
					else
						++it;
				}
			}

			views_to_remove.clear();
		}
	}

	return result;
}

}
}
