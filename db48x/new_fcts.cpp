

#include "new_fcts.h"


#include "arithmetic.h"
#include "array.h"
#include "bignum.h"
#include "compare.h"
#include "conditionals.h"
#include "decimal.h"
#include "expression.h"
#include "fraction.h"
#include "integer.h"
#include "integrate.h"
#include "list.h"
#include "logical.h"
#include "polynomial.h"
#include "range.h"
#include "solve.h"
#include "tag.h"
#include "unit.h"
#include "variables.h"

#include "SEGGER_RTT.h"


inline void RTT_vprintf_cr_time( const char * sFormat, ...);

algebraic_p testnfct::evaluate(id op, algebraic_g args[], uint arity)
{

 algebraic_g &a = args[0];  
// algebraic_g &aa = args[0];  
    algebraic_g &b = args[1];  
      
    // Create 1.5 constant  
    algebraic_g one_point_five = decimal::make(15, -1);  // 15 * 10^-1 = 1.5  
      

//
double d = 1.1;


    // Compute 1.5 * a  
    algebraic_g result = one_point_five * a;  
      
    // Subtract b  
    result = result - b;  
      
    return result;  
}

/*
if (algebraic::hwfp_promotion(aa))  
{  
    if (hwfloat_p hf = aa->as<hwfloat>())  
        d = hf->as_double();  
    else if (hwdouble_p hd = aa->as<hwdouble>())  
        d = hd->as_double();  
}  else
    {  
        rt.type_error();  
        return nullptr;  
    }  
    d*=1000.;
RTT_vprintf_cr_time("Fct1 d=%d", int32_t (d));



algebraic_p testnfct_old::evaluate(id op, algebraic_g args[], uint arity)
{

// Get the list argument  
    list_p lst = args[0]->as<list>();  
    if (!lst || lst->size() != 3)  
    {  
        rt.type_error();  
        return nullptr;  
    }  
  
    // Extract elements from list  
    object_p obj0 = lst->at(0);  
    object_p obj1 = lst->at(1);  
    object_p obj2 = lst->at(2);  
  
    // Validate types  
        if (text_p tobj = obj0->as<text>())
        {
               
        }
        else
        {
            rt.type_error();
            return nullptr; 
        }
    if ( !obj1->is_integer() || !obj2->is_real())  
    {  
        rt.type_error();  
        return nullptr;  
    }  
double d = 1.1;

    // Convert to appropriate types  
    text_p str = text_p(obj0);  
    integer_p int_val = integer_p(obj1);  
    algebraic_p float_val = algebraic_p(obj2);  
algebraic_g guarded = float_val; 

// Option 1: Promote to hardware FP and extract  
if (algebraic::hwfp_promotion(guarded))  
{  
    if (hwfloat_p hf = guarded->as<hwfloat>())  
        d = hf->as_double();  
    else if (hwdouble_p hd = guarded->as<hwdouble>())  
        d = hd->as_double();  
}  
// Option 2: If it's a decimal, convert directly  
else if (decimal_p dec = guarded->as<decimal>())  
{  
    d = dec->to_double();  
}  
else  
{  
    rt.type_error();  
    return nullptr; // or handle error  
}
  
      
hwfloat_p hf = hwfloat::make(static_cast<float>(d*2.1));

    return hf;  

}
*/