<?php

interface DefaultMultistepFormInterface {

  /**
  * Builds and return the form structure
  * @return array
  *   The form array. It must contain submit elements for Previous and Next, and final Submit)
  */
  function form();

  /**
   * Initializes the form_state variable with the steps
   */
  function init_form();
  /**
   * Submit callback for the next button. This will need to be called from the "real" form submit callback for the next button 
   */
  function next();


  /**
   * Submit callback for the next button. This will need to be called from the real form submit callback for the previous button
   */
  function previous();

  /**
   * Submit callback for the finish button
   */
  function finish();

  /** 
   * Invoked after the last step. Finalizes the form (e.g. saves entities)
   */
  function form_finalizer();

  /**
   * Step definition in the form
   * Each step is an array keyed by the step number, pointing at an array with the following keys:
   *    type
   *    form - the function that return the content of the form for this step
   *    title
   *    validation - the name of the function used to validate
   *    next hook - the function called after the validation, before moving to the next step. This performs any entity creation aetc
   *    finalizer hook - a function to call when submiting after the last step of the form
   */   
  function _form_steps_info(); 

}

/** 
 * A simple form class that implements the interface
 */
class DefaultMultistepForm implements DefaultMultistepFormInterface {
  
  public $form;
  public $form_state;
  
  function __construct($form, &$form_state) {
    $this->form = $form;
    $this->form_state = &$form_state;
    $this->form_state['controller_name'] = get_class($this);
  }

  /**
  *  Implements DefaultMultistepFormInterface::form
  * @return array
  *   The form array. It must contain submit elements for Previous and Next, and final Submit)
  */
  public function form() {
    if (empty($this->form_state['step'])) {
      $this->init_form();
      $this->form_state['step'] = 1;
    }

    $this->form = array();

    $step = $this->form_state['step'];
    drupal_set_title(t('!title' , array('!title' => $this->form_state['step_information'][$step]['title'])));
    
    $this->{$this->form_state['step_information'][$step]['form']}();
    $count = count($this->form_state['step_information']);
    $this->form['subtitle'] = array( // Needs to theme the subtitle!
      '#markup' => "<h3>Step $step of $count</h3><br>",
      '#weight' => -99,
    );

    if ($step > 1) {
      $this->form['prev'] = array(
        '#type' => 'submit',
        '#value' => t('Previous'),
        '#name' => 'prev',
        '#weight' => 98,
        '#limit_validation_errors' => array(),
        '#submit' => array('multistep_form_form_submit'),
      );
    }
    if ($step < $count) {
      $this->form['next'] = array(
        '#type' => 'submit',
        '#value' => t('Next'),
        '#name' => 'next',
        '#weight' => 99,
      );
    } else {
      $this->form['finish'] = array(
        '#type' => 'submit',
        '#name' => 'finish',
        '#value' => 'Finish',
        '#weight' => 99,
      );
    }
    return $this->form; 
  }

  public function init_form() {
    $this->form_state['step_information'] = $this->_form_steps_info();
  }

  /**
   * Implements DefaultMultistepFormInterface::next
   * 
   * Submit callback for the next button. This will need to be called from the "real" form submit callback for the next button 
   */
  public function next() {
    $current_step = &$this->form_state['step'];
    // Store values in form_state to access later
    $this->form_state['step_information'][$current_step]['stored_values'] = $this->form_state['values'];
    // Invoke the additional hook
    if (!empty($this->form_state['step_information'][$current_step]['next hook'])) {
      $this->{$this->form_state['step_information'][$current_step]['next hook']}();
    }
    if ($current_step < count($this->form_state['step_information'])) {
      $current_step++;
      if (!empty($this->form_state['step_information'][$current_step]['stored_values'])) {
        // Restore the values for the next step of the form
        $this->form_state['values'] = $this->form_state['step_information'][$current_step]['stored_values'];
      }
      else {
        // clean up the values in form_state for the next step
        $this->form_state['values'] = array();
      } 
      // Force rebuild with next step.
      $this->form_state['rebuild'] = TRUE;
    }
  }


  /**
   * Implements DefaultMultistepFormInterface::previous
   * 
   * Submit callback for the next button. This will need to be called from the real form submit callback for the previous button
   */
  public function previous() {
    $current_step = &$this->form_state['step'];
    if ($current_step > 1) {
      $current_step--;
      $this->form_state['values'] = $this->form_state['step_information'][$current_step]['stored_values'];
    }
    $this->form_state['rebuild'] = TRUE;
  }

  /**
   * Implements DefaultMultistepFormInterface::finish
   * 
   * Submit callback for the finish button
   */
  public function finish() {
    $current_step = &$this->form_state['step'];
    $this->form_state['step_information'][$current_step]['stored_values'] = $this->form_state['values'];
    // Invoke the additional hook
    if (!empty($this->form_state['step_information'][$current_step]['next hook'])) {
      $this->{$this->form_state['step_information'][$current_step]['next hook']}();
    }
    // Call the finalizer function
    $this->form_finalizer();
  }


  /**
   * Implements DefaultMultistepFormInterface::form_finalizer
   */
  public function form_finalizer() {
    foreach ($this->form_state['step_information'] as $step_info) {
      if (isset($step_info['finalizer hook'])) {
        $finalizer = $step_info['finalizer hook'];
        $this->$finalizer();
      }
    }
  }

  public function _form_steps_info() {
  }
  
}

/** 
 * An example on how to extend DefaultMultistepForm
 */
class ExampleMultistepForm extends DefaultMultistepForm {
  /**
   * Provides default info about each step. The form will build step_information using this info
   */
  public function _form_steps_info() {
    return array (
      '1' => array (
        'type' => 'step_1_form',
        'form' => 'step_1_form',
        'title' => 'Step 1',
        'validation' => array(
          'step_1_validation'  // These are validation functions to call when any submit button is pressed (next, previous, etc)
        ),
        'next hook' => 'step_1_next_hook',
        'finalizer hook' => 'step_1_finalizer_hook',
      ),
      
    );
  }

  /** 
   * The class includes the functions that generate the form structure for each step. This examples provides one step only
   * 
   */
  public function step_1_form() {
    $values = isset($this->form_state['values']) ? $this->form_state['values'] : array(); // This is automatically populated by the next or previous button
    $this->form['name'] = array(
      '#type' => 'textfield',
      '#title' => 'Enter your name',
      '#required' => TRUE,
      '#default_value' => isset($values['name']) ? $values['name'] : '',
    );
    $this->form['color'] = array(
      '#type' => 'textfield',
      '#title' => 'Enter your favorite color',
      '#required' => TRUE,
      '#default_value' => isset($values['color']) ? $values['color'] : '',
    );
  }

  public function step_1_form_validate() {
    $values = $this->form_state['values'];
    if (strtolower($values['color']) == 'blue') {
      form_set_error('color', t('You are not allowed to choose blue!'));
    }
  }

  public function step_1_form_next_hook() {
    $values = $this->form_state['values'];
    drupal_set_message(t('Your name is @name and you picked @color', array('@name' => $values['name'], '@color' => $values['color'])));
  }

  public function step_1_form_finalizer_hook() {
    // Nothing to do here.
  }

}