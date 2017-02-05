#include "BaseTag.h"

BaseTag::BaseTag()
{
	this->index = 0;
	this->standalone = false;
	this->block_level = false;
	this->parent = nullptr;
}

BaseTag* BaseTag::copy()
{
	BaseTag* new_tag = this->specificCopy();
	new_tag->attributes = this->attributes;
	return new_tag;
}

BaseTag* BaseTag::specificCopy()
{
	return new BaseTag;
}

int BaseTag::accumulateAttribute(AttributeType attribute)
{
	int sum = 0;
	BaseTag* current = this->getParent();
	while (current != nullptr)
	{
		sum += safe_stoi(current->getAttribute(attribute));
		current = current->getParent();
	}
	return sum;
}

TagType BaseTag::stringToType(string type)
{
	if (type == "br") return BR;
	else if (type == "b") return B;
	else if (type == "i") return I;
	else if (type == "body") return BODY;
	else if (type == "p") return P;
	else if (type == "h1") return H1;
	else if (type == "h2") return H2;
	else if (type == "h3") return H3;
	else if (type == "h4") return H4;
	else if (type == "h5") return H5;
	else if (type == "h6") return H6;
	else if (type == "div") return DIV;
	else if (type == "a") return A;
}

BaseTag* BaseTag::addChild(BaseTag * child)
{
	if (child != nullptr)
	{
		this->children.push_back(child);
		child->parent = this;
		child->index = this->children.size() - 1;
		return child;
	}
	return nullptr;
}

Point BaseTag::proccess(ProccessContext context)
{
	// Specific Proccess
	this->specificProccess(context);
	// Block Level Element
	if (this->block_level)
	{
		context.current_starting_point.y += context.max_height;
		context.current_starting_point.x = context.indent;
		context.max_height = 0;
	}
	else
	{
		this->setAttribute(MARGIN_TOP, "0");
		this->setAttribute(MARGIN_BOTTOM, "0");
	}
	// Margin
	context.location_addition.x += safe_stoi(this->getAttribute(MARGIN_LEFT));
	context.location_addition.y += safe_stoi(this->getAttribute(MARGIN_TOP));
	// implement context
	this->location = context.current_starting_point;
	this->location_addition = context.location_addition;
	ProccessContext inner_context = context;
	// Padding
	inner_context.indent += safe_stoi(this->getAttribute(PADDING_LEFT));
	inner_context.current_starting_point.x += inner_context.indent;
	inner_context.current_starting_point.y += safe_stoi(this->getAttribute(PADDING_TOP));
	// Recursive Proccess
	Point children_bounding_box;
	Point starting_location = this->absoluteLocation();
	for (vector<BaseTag*>::const_iterator child_ptr = this->getFirstChild(); 
		child_ptr != this->getEndChild(); child_ptr++)
	{
		this->passStyle(*child_ptr); // pass in context
		inner_context.current_starting_point = (*child_ptr)->proccess(inner_context);
		Point d = (*child_ptr)->absoluteLocation() + (*child_ptr)->bounding_box;
		d = d - starting_location;
		children_bounding_box.x = (d.x > children_bounding_box.x) ? d.x : children_bounding_box.x;
		children_bounding_box.y = (d.y > children_bounding_box.y) ? d.y : children_bounding_box.y;
	}
	// setting bounding box
	if (this->bounding_box == Point())
	{
		this->bounding_box = children_bounding_box;
		if (this->block_level)
		{
			string h = this->getAttribute(HEIGHT);
			string w = this->getAttribute(WIDTH);
			if (w != "")
			{
				if (w.at(0) != '%') this->bounding_box.x = safe_stoi(w);
			}
			if (h != "")
			{
				if (h.at(0) != '%') this->bounding_box.y = safe_stoi(h);
			}
		}
		bounding_box.x += safe_stoi(this->getAttribute(PADDING_RIGHT));// +safe_stoi(this->getAttribute(PADDING_LEFT));
		bounding_box.y += safe_stoi(this->getAttribute(PADDING_BOTTOM));// +safe_stoi(this->getAttribute(PADDING_TOP));
	}
	context.current_starting_point.x += this->bounding_box.x;
	// add padding to bounding box
	bounding_box.x += safe_stoi(this->getAttribute(PADDING_RIGHT));
	bounding_box.y += safe_stoi(this->getAttribute(PADDING_BOTTOM));
	// set next starting point
	context.max_height = (this->bounding_box.y > context.max_height) ? this->bounding_box.y : context.max_height;
	context.current_starting_point.x += safe_stoi(this->getAttribute(MARGIN_RIGHT)) + 
		safe_stoi(this->getAttribute(MARGIN_LEFT));
	context.current_starting_point.y += safe_stoi(this->getAttribute(MARGIN_BOTTOM)) + 
		safe_stoi(this->getAttribute(MARGIN_TOP));
	if (this->block_level)
	{
		context.current_starting_point.y += context.max_height;
		context.current_starting_point.x = context.indent;
		context.max_height = 0;
	}
	return context.current_starting_point;
}

void BaseTag::passStyle(BaseTag * child)
{
	child->addLocationAddition(this->location_addition.x, this->location_addition.y);
	for (int i = AttributeType::PASSABLE + 1; i < AttributeType::LAST; i++)
	{
		this->passAttribute(child, (AttributeType)i);
	}
}

void BaseTag::passAttribute(BaseTag * child, AttributeType attr)
{
	if (child->getAttribute(attr) == "")
	{
		child->setAttribute(attr, this->getAttribute(attr));
	}
}

void BaseTag::addLocation(int x, int y)
{
	this->location.x += x;
	this->location.y += y;
}

void BaseTag::setLocation(int x, int y)
{
	this->location.x = x;
	this->location.y = y;
}

void BaseTag::addLocationAddition(int x, int y)
{
	this->location_addition.x += x;
	this->location_addition.y += y;
}

void BaseTag::setLocationAddition(int x, int y)
{
	this->location_addition.x = x;
	this->location_addition.y = y;
}

int BaseTag::getRowHeight()
{
	sf::Text text;
	int font_size = safe_stoi(this->getAttribute(FontSize));
	return font_size - 2;
}

int BaseTag::safe_stoi(string str, size_t * index, int base)
{
	if (str != "")
	{
		return stoi(str, index, base);
	}
	return 0;
}

void BaseTag::draw(DrawContext& context)
{
	if (this->location_addition != Point())
	{
		this->addLocation(this->location_addition.x, this->location_addition.y);
		this->location_addition = Point();
	}
	// Basic Draw
	if (this->block_level)
	{
		string w = this->getAttribute(WIDTH);
		if (w == "") this->bounding_box.x = context.window.getSize().x
			- this->location.x - safe_stoi(this->getAttribute(MARGIN_RIGHT)) - accumulateAttribute(PADDING_RIGHT) - accumulateAttribute(MARGIN_RIGHT);
		else
		{
			if (w.at(0) == '%')
			{
				this->bounding_box.x = context.window.getSize().x
					- this->location.x - accumulateAttribute(PADDING_RIGHT) - accumulateAttribute(MARGIN_RIGHT);
				this->bounding_box.x = this->bounding_box.x * safe_stoi(w.substr(1)) / 100;
			}
			
		}
		string h = this->getAttribute(HEIGHT);
		if (h != "")
		{
			if (h.at(0) == '%') this->bounding_box.y = this->bounding_box.y * safe_stoi(h.substr(1)) / 100;
		}

	}
	sf::RectangleShape rectangle(sf::Vector2f(this->bounding_box.x, this->bounding_box.y));
	rectangle.setPosition(this->location.x, this->location.y);
	rectangle.setFillColor(hexToColor(this->getAttribute(BackgroundColor)));
	context.window.draw(rectangle);
	// Specific Draw
	this->specificDraw(context);
	// Recursive Draw
	for (vector<BaseTag*>::const_iterator child_ptr = this->getFirstChild(); child_ptr != this->getEndChild(); child_ptr++)
	{
		(*child_ptr)->draw(context);
	}
}

sf::Color BaseTag::hexToColor(string hex)
{
	if (hex == "" || hex.size() != 6)
	{
		return sf::Color::Transparent;
	}
	string red, green, blue;
	red = hex.substr(0, 2);
	green = hex.substr(2, 2);
	blue = hex.substr(4, 2);
	sf::Color color = sf::Color(
		safe_stoi(red, 0, 16),
		safe_stoi(green, 0, 16),
		safe_stoi(blue, 0, 16)
		);
	return color;
}

string BaseTag::getAttribute(AttributeType type)
{
	try
	{
		return this->attributes.at(type);
	}
	catch (...)
	{
		return "";
	}
}
