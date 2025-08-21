/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

function byte_test(): void {
  let byte_: byte = 42;
  let Byte_: Byte = new Byte(42 as byte);

  {
    // Casting to primitive types
    let byte_byte   = byte_.toByte();
    let byte_short  = byte_.toShort();
    let byte_char   = byte_.toChar();
    let byte_int    = byte_.toInt();
    let byte_long   = byte_.toLong();
    let byte_float  = byte_.toFloat();
    let byte_double = byte_.toDouble();

    let Byte_byte   = Byte_.toByte();
    let Byte_short  = Byte_.toShort();
    let Byte_int    = Byte_.toInt();
    let Byte_long   = Byte_.toLong();
    let Byte_float  = Byte_.toFloat();
    let Byte_double = Byte_.toDouble();
  }

  {
    //  Casting to reference type
    let byte_Byte   = byte_ as Byte;
    let Byte_Byte   = Byte_ as Byte;
    let Byte_Object = Byte_ as Object;
  }
}

function short_test(): void {
  let short_: short = 42;
  let Short_: Short = new Short(42 as short);

  {
    // Casting to primitive types
    let short_byte   = short_.toByte();
    let short_short  = short_.toShort();
    let short_char   = short_.toChar();
    let short_int    = short_.toInt();
    let short_long   = short_.toLong();
    let short_float  = short_.toFloat();
    let short_double = short_.toDouble();

    let Short_short  = Short_.toShort();
    let Short_int    = Short_.toInt();
    let Short_long   = Short_.toLong();
    let Short_float  = Short_.toFloat();
    let Short_double = Short_.toDouble();
  }

  {
    //  Casting to reference type
    let short_Short   = short_ as Short;
    let Short_Short   = Short_ as Short;
    let Short_Object  = Short_ as Object;
  }
}

function char_test(): void {
  let char_: char = Int.toChar(42);
  let Char_: Char = new Char(Int.toChar(42));

  {
    // Casting to primitive types
    let char_byte   = char_.toByte();
    let char_short  = char_.toShort();
    let char_char   = char_.toChar();
    let char_int    = char_.toInt();
    let char_long   = char_.toLong();
    let char_float  = char_.toFloat();
    let char_double = char_.toDouble();

    let Char_char   = Char_.toChar();
    let Char_int    = Char_.toInt();
    let Char_long   = Char_.toLong();
    let Char_float  = Char_.toFloat();
    let Char_double = Char_.toDouble();
  }

  {
    //  Casting to reference type
    let char_Char   = char_ as Char;
    let Char_Char   = Char_ as Char;
    let Char_Object = Char_ as Object;
  }
}

function int_test(): void {
  let int_: int = 42;
  let Int_: Int = new Int(42 as int);

  {
    // Casting to primitive types
    let int_byte    = int_.toByte();
    let int_short   = int_.toShort();
    let int_char    = int_.toChar();
    let int_int     = int_.toInt();
    let int_long    = int_.toLong();
    let int_float   = int_.toFloat();
    let int_double  = int_.toDouble();

    let Int_int     = Int_.toInt();
    let Int_long    = Int_.toLong();
    let Int_float   = Int_.toFloat();
    let Int_double  = Int_.toDouble();
  }

  {
    //  Casting to reference type
    let int_Int     = int_ as Int;
    let Int_Int     = Int_ as Int;
    let Int_Object  = Int_ as Object;
  }
}

function long_test(): void {
  let long_: long = 42;
  let Long_: Long = new Long(42 as long);

  {
    // Casting to primitive types
    let long_byte   = long_.toByte();
    let long_short  = long_.toShort();
    let long_char   = long_.toChar();
    let long_int    = long_.toInt();
    let long_long   = long_.toLong();
    let long_float  = long_.toFloat();
    let long_double = long_.toDouble();

    let Long_long   = Long_.toLong();
    let Long_float  = Long_.toFloat();
    let Long_double = Long_.toDouble();
  }

  {
    //  Casting to reference type
    let long_Long   = long_ as Long;
    let Long_Long   = Long_ as Long;
    let Long_Object = Long_ as Object;
  }
}

function float_test(): void {
  let float_: float = 42;
  let Float_: Float = new Float(42 as float);

  {
    // Casting to primitive types
    let float_byte   = float_.toByte();
    let float_short  = float_.toShort();
    let float_int    = float_.toInt();
    let float_long   = float_.toLong();
    let float_float  = float_.toFloat();
    let float_double = float_.toDouble();

    let Float_float  = Float_.toFloat();
    let Float_double = Float_.toDouble();
  }

  {
    //  Casting to reference type
    let float_Float   = float_ as Float;
    let Float_Float   = Float_ as Float;
    let Float_Object  = Float_ as Object;
  }
}

function double_test(): void {
  let double_: double = 42;
  let Double_: Double = new Double(42 as double);

  {
    // Casting to primitive types
    let double_byte   = double_.toByte();
    let double_short  = double_.toShort();
    let double_int    = double_.toInt();
    let double_long   = double_.toLong();
    let double_float  = double_.toFloat();
    let double_double = double_.toDouble();

    let Double_double = Double_.toDouble();
  }

  {
    //  Casting to reference type
    let double_Double   = double_ as Double;
    let Double_Double   = Double_ as Double;
    let Double_Object   = Double_ as Object;
  }
}

function boolean_test(): void {
  let boolean_: boolean = true;
  let Boolean_: Boolean = new Boolean(true);

  {
    // Casting to primitive types
    let boolean_boolean = boolean_ as boolean;
    let Boolean_boolean = Boolean_ as boolean;
  }

  {
    // Casting to reference type
    let boolean_Boolean = boolean_ as Boolean;
    let Boolean_Boolean = Boolean_ as Boolean;
    let Boolean_Object  = Boolean_ as Object;
  }
}
